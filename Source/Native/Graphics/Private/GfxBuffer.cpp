#include "GfxBuffer.h"
#include "GfxResource.h"
#include "GfxDevice.h"
#include "MathUtils.h"
#include "Debug.h"
#include <assert.h>
#include <stdexcept>

namespace march
{
    GfxAABuffer::GfxAABuffer(const std::string& name, const GfxBufferDesc& desc, GfxResourceAllocator* allocator)
        : m_Stride(desc.Stride)
        , m_Count(desc.Count)
        , m_CpuAccessFlags(desc.CpuAccessFlags)
        , m_MappedData(nullptr)
    {
        UINT64 width = static_cast<UINT64>(m_Stride) * static_cast<UINT64>(m_Count);
        D3D12_RESOURCE_FLAGS flags = desc.UnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        m_Resource = allocator->Allocate(name, &CD3DX12_RESOURCE_DESC::Buffer(width, flags), desc.InitialState);

        if (desc.CpuAccessFlags != GfxBufferAccessFlags::None)
        {
            assert(CD3DX12_HEAP_PROPERTIES(allocator->GetHeapProperties()).IsCPUAccessible());

            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map
            bool read = (desc.CpuAccessFlags & GfxBufferAccessFlags::Read) == GfxBufferAccessFlags::Read;
            GFX_HR(m_Resource->GetD3DResource()->Map(0, read ? nullptr : &CD3DX12_RANGE(0, 0), &m_MappedData));
        }
    }

    GfxAABuffer::~GfxAABuffer()
    {
        if (m_MappedData != nullptr)
        {
            m_MappedData = nullptr;

            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-unmap
            bool write = (m_CpuAccessFlags & GfxBufferAccessFlags::Write) == GfxBufferAccessFlags::Write;
            m_Resource->GetD3DResource()->Unmap(0, write ? nullptr : &CD3DX12_RANGE(0, 0));
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxAABuffer::GetGpuVirtualAddress(uint32_t index) const
    {
        auto offset = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(index) * static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_Stride);
        return m_Resource->GetD3DResource()->GetGPUVirtualAddress() + offset;
    }

    void* GfxAABuffer::GetDataPointer(uint32_t index) const
    {
        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of range");
        }

        uint8_t* p = static_cast<uint8_t*>(m_MappedData);
        return &p[index * m_Stride];
    }

    D3D12_VERTEX_BUFFER_VIEW GfxAABuffer::GetVbv() const
    {
        D3D12_VERTEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(0);
        view.SizeInBytes = static_cast<UINT>(GetSize());
        view.StrideInBytes = static_cast<UINT>(GetStride());
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW GfxAABuffer::GetIbv() const
    {
        assert(GetStride() == 2u || GetStride() == 4u);

        D3D12_INDEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(0);
        view.SizeInBytes = static_cast<UINT>(GetSize());
        view.Format = GetStride() == 2u ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
        return view;
    }

    GfxUploadMemory::GfxUploadMemory(GfxUploadBuffer* buffer, uint32_t offset, uint32_t stride, uint32_t count)
        : m_Buffer(buffer), m_Offset(offset), m_Stride(stride), m_Count(count)
    {
    }

    uint8_t* GfxUploadMemory::GetMappedData(uint32_t index) const
    {
        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of range");
        }

        uint8_t* p = m_Buffer->GetMappedData(0);
        return &p[m_Offset + index * m_Stride];
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxUploadMemory::GetGpuVirtualAddress(uint32_t index) const
    {
        using Addr = D3D12_GPU_VIRTUAL_ADDRESS;

        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of range");
        }

        Addr addr = m_Buffer->GetGpuVirtualAddress(0);
        Addr offset = static_cast<Addr>(m_Offset) + static_cast<Addr>(index) * static_cast<Addr>(m_Stride);
        return addr + offset;
    }

    uint32_t GfxUploadMemory::GetD3D12ResourceOffset(uint32_t index) const
    {
        return m_Offset + index * m_Stride;
    }

    ID3D12Resource* GfxUploadMemory::GetD3D12Resource() const
    {
        return m_Buffer->GetD3D12Resource();
    }

    GfxUploadMemoryAllocator::GfxUploadMemoryAllocator(GfxDevice* device)
        : m_Device(device), m_AllocateOffset(0), m_PageCounter(0)
        , m_UsedPages{}, m_LargePages{}, m_ReleaseQueue{}
    {
    }

    void GfxUploadMemoryAllocator::BeginFrame()
    {
    }

    void GfxUploadMemoryAllocator::EndFrame(uint64_t fenceValue)
    {
        for (std::unique_ptr<GfxUploadBuffer>& p : m_UsedPages)
        {
            m_ReleaseQueue.emplace(fenceValue, std::move(p));
        }

        m_UsedPages.clear();
        m_LargePages.clear();
    }

    GfxUploadMemory GfxUploadMemoryAllocator::Allocate(uint32_t size, uint32_t count, uint32_t alignment)
    {
        uint32_t stride = MathUtils::AlignUp(size, alignment);
        uint32_t totalSize = stride * count;

        if (totalSize > PageSize)
        {
            std::string name = "GfxUploadMemoryPage (Large)";
            m_LargePages.emplace_back(std::make_unique<GfxUploadBuffer>(m_Device, name, stride, count, true));

            LOG_TRACE("Create %s; Size: %d", name.c_str(), totalSize);
            return GfxUploadMemory(m_LargePages.back().get(), 0, stride, count);
        }

        uint32_t offset = MathUtils::AlignUp(m_AllocateOffset, alignment);

        if (m_UsedPages.empty() || offset + totalSize > PageSize)
        {
            if (!m_ReleaseQueue.empty() && m_Device->IsGraphicsFenceCompleted(m_ReleaseQueue.front().first))
            {
                m_UsedPages.emplace_back(std::move(m_ReleaseQueue.front().second));
                m_ReleaseQueue.pop();
            }
            else
            {
                std::string name = "GfxUploadMemoryPage" + std::to_string(m_PageCounter++);
                m_UsedPages.emplace_back(std::make_unique<GfxUploadBuffer>(m_Device, name, PageSize, 1, true));

                LOG_TRACE("Create %s; Size: %d", name.c_str(), PageSize);
            }

            offset = 0;
        }

        m_AllocateOffset = offset + totalSize;
        return GfxUploadMemory(m_UsedPages.back().get(), offset, stride, count);
    }
}
