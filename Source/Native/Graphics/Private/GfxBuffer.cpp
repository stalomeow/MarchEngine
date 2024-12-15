#include "GfxBuffer.h"
#include "GfxCommand.h"
#include "GfxDevice.h"
#include <stdexcept>

namespace march
{
    GfxBuffer::GfxBuffer(GfxDevice* device, const std::string& name, const GfxBufferDesc& desc, GfxAllocator allocator)
        : m_Resource{}
        , m_MappedData(nullptr)
    {
        GfxCompleteResourceAllocator* resAllocator = device->GetAllocator(allocator);
        UINT64 width = static_cast<UINT64>(desc.SizeInBytes);
        D3D12_RESOURCE_FLAGS flags = desc.UnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        m_Resource = resAllocator->Allocate(name, &CD3DX12_RESOURCE_DESC::Buffer(width, flags), desc.InitialState);

        TryMapData(resAllocator);
    }

    GfxBuffer::GfxBuffer(GfxDevice* device, uint32_t sizeInBytes, uint32_t dataPlacementAlignment, GfxSubAllocator allocator)
        : m_Resource{}
        , m_MappedData(nullptr)
    {
        GfxBufferSubAllocator* subAllocator = device->GetSubAllocator(allocator);
        m_Resource = subAllocator->Allocate(sizeInBytes, dataPlacementAlignment);

        TryMapData(subAllocator);
    }

    GfxBuffer::~GfxBuffer()
    {
        if (m_MappedData)
        {
            m_MappedData = nullptr;

            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-unmap
            GetResource()->GetD3DResource()->Unmap(0, nullptr);
        }
    }

    void GfxBuffer::TryMapData(GfxResourceAllocator* allocator)
    {
        if (allocator->IsHeapCpuAccessible())
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map
            GFX_HR(GetResource()->GetD3DResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxBuffer::GetGpuVirtualAddress(uint32_t offset) const
    {
        if (offset >= GetSize())
        {
            throw std::out_of_range("GfxBuffer::GetGpuVirtualAddress: offset out of range");
        }

        D3D12_GPU_VIRTUAL_ADDRESS address = GetResource()->GetD3DResource()->GetGPUVirtualAddress();
        return address + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_Resource.GetBufferOffset() + offset);
    }

    void GfxBuffer::SetData(uint32_t destOffset, const void* src, uint32_t sizeInBytes)
    {
        if (GetSize() - destOffset < sizeInBytes)
        {
            throw std::out_of_range("GfxBuffer::SetData: size out of range");
        }

        if (m_MappedData)
        {
            memcpy(m_MappedData + destOffset, src, static_cast<size_t>(sizeInBytes));
        }
        else
        {
            GfxDevice* device = GetDevice();
            GfxCommandContext* context = device->RequestContext(GfxCommandType::Direct);

            GfxBuffer tempUpload{ device, sizeInBytes, 0, GfxSubAllocator::TempUpload };
            tempUpload.SetData(0, src, sizeInBytes);

            D3D12_RESOURCE_STATES currentState = GetResource()->GetState();
            context->TransitionResource(GetResource(), D3D12_RESOURCE_STATE_COPY_DEST);
            context->TransitionResource(tempUpload.GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE);
            context->FlushResourceBarriers();

            context->GetCommandList()->CopyBufferRegion(
                GetResource()->GetD3DResource(),
                static_cast<UINT64>(m_Resource.GetBufferOffset() + destOffset),
                tempUpload.GetResource()->GetD3DResource(),
                static_cast<UINT64>(tempUpload.m_Resource.GetBufferOffset()),
                static_cast<UINT64>(sizeInBytes));

            context->TransitionResource(GetResource(), currentState);
            context->SubmitAndRelease().WaitOnCpu();
        }
    }
}
