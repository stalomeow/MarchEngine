#include "GfxBuffer.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "GfxFence.h"
#include "MathHelper.h"
#include "Debug.h"
#include <stdexcept>

namespace march
{
    GfxBuffer::GfxBuffer(GfxDevice* device, D3D12_HEAP_TYPE heapType, const std::string& name, uint32_t stride, uint32_t count)
        : GfxResource(device, D3D12_RESOURCE_STATE_COMMON), m_Stride(stride), m_Count(count)
    {
        UINT64 width = static_cast<UINT64>(stride) * static_cast<UINT64>(count);

        GFX_HR(device->GetD3D12Device()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(heapType),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(width),
            m_State, nullptr, IID_PPV_ARGS(&m_Resource)));
        SetD3D12ResourceName(name);
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxBuffer::GetGpuVirtualAddress(uint32_t index) const
    {
        auto offset = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(index) * static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_Stride);
        return m_Resource->GetGPUVirtualAddress() + offset;
    }

    GfxUploadBuffer::GfxUploadBuffer(GfxDevice* device, const std::string& name, uint32_t stride, uint32_t count, bool readable)
        : GfxBuffer(device, D3D12_HEAP_TYPE_UPLOAD, name, stride, count), m_MappedData(nullptr)
    {
        GFX_HR(m_Resource->Map(0, readable ? nullptr : &CD3DX12_RANGE(0, 0), &m_MappedData));
    }

    GfxUploadBuffer::~GfxUploadBuffer()
    {
        m_Resource->Unmap(0, nullptr);
        m_MappedData = nullptr;
    }

    uint8_t* GfxUploadBuffer::GetMappedData(uint32_t index) const
    {
        uint8_t* p = reinterpret_cast<uint8_t*>(m_MappedData);
        return &p[index * m_Stride];
    }

    GfxConstantBuffer::GfxConstantBuffer(GfxDevice* device, const std::string& name, uint32_t dataSize, uint32_t count, bool readable)
        : GfxUploadBuffer(device, name, GetAlignedSize(dataSize), count, readable)
    {
    }

    void GfxConstantBuffer::CreateView(uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) const
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = GetGpuVirtualAddress(index);
        desc.SizeInBytes = static_cast<UINT>(m_Stride);
        m_Device->GetD3D12Device()->CreateConstantBufferView(&desc, destDescriptor);
    }

    uint32_t GfxConstantBuffer::GetAlignedSize(uint32_t size)
    {
        return MathHelper::AlignUp(size, Alignment);
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
        uint32_t stride = MathHelper::AlignUp(size, alignment);
        uint32_t totalSize = stride * count;

        if (totalSize > PageSize)
        {
            std::string name = "GfxUploadMemoryPage (Large)";
            m_LargePages.emplace_back(std::make_unique<GfxUploadBuffer>(m_Device, name, stride, count, true));

            DEBUG_LOG_INFO("Create %s; Size: %d", name.c_str(), totalSize);
            return GfxUploadMemory(m_LargePages.back().get(), 0, stride, count);
        }

        uint32_t offset = MathHelper::AlignUp(m_AllocateOffset, alignment);

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

                DEBUG_LOG_INFO("Create %s; Size: %d", name.c_str(), PageSize);
            }

            offset = 0;
        }

        m_AllocateOffset = offset + totalSize;
        return GfxUploadMemory(m_UsedPages.back().get(), offset, stride, count);
    }
}
