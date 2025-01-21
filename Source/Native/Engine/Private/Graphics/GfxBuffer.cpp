#include "pch.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxCommand.h"
#include "Engine/Graphics/GfxDevice.h"
#include <stdexcept>

namespace march
{
    RefCountPtr<GfxResource> GfxBufferResource::GetUnderlyingResource(GfxBufferElement element) const
    {
        switch (element)
        {
        case GfxBufferElement::Data:
            return m_DataResource;
        case GfxBufferElement::Counter:
            return m_CounterResource;
        default:
            throw GfxException("Invalid buffer element");
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxBufferResource::GetGpuVirtualAddress(GfxBufferElement element) const
    {
        ID3D12Resource* resource;
        uint32_t offsetInBytes;

        switch (element)
        {
        case GfxBufferElement::Data:
            resource = m_DataResource->GetD3DResource();
            offsetInBytes = m_DataResourceOffsetInBytes;
            break;
        case GfxBufferElement::Counter:
            resource = m_CounterResource->GetD3DResource();
            offsetInBytes = m_CounterResourceOffsetInBytes;
            break;
        default:
            throw GfxException("Invalid buffer element");
        }

        D3D12_GPU_VIRTUAL_ADDRESS baseAddress = resource->GetGPUVirtualAddress();
        return baseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(offsetInBytes);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxBufferResource::GetUav()
    {
        if (!m_UavDescriptor)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

            switch (m_Desc.UnorderedAccessMode)
            {
            case GfxBufferUnorderedAccessMode::Structured:
                desc.Format = DXGI_FORMAT_UNKNOWN;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataResourceOffsetInBytes / m_Desc.Stride);
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.Count);
                desc.Buffer.StructureByteStride = static_cast<UINT>(m_Desc.Stride);
                desc.Buffer.CounterOffsetInBytes = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                break;
            case GfxBufferUnorderedAccessMode::StructuredWithCounter:
                desc.Format = DXGI_FORMAT_UNKNOWN;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataResourceOffsetInBytes / m_Desc.Stride);
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.Count);
                desc.Buffer.StructureByteStride = static_cast<UINT>(m_Desc.Stride);
                desc.Buffer.CounterOffsetInBytes = m_CounterResourceOffsetInBytes;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                break;
            case GfxBufferUnorderedAccessMode::Raw:
                desc.Format = DXGI_FORMAT_R32_TYPELESS;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataResourceOffsetInBytes / sizeof(uint32_t));
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.Count * m_Desc.Stride / sizeof(uint32_t));
                desc.Buffer.StructureByteStride = 0;
                desc.Buffer.CounterOffsetInBytes = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                break;
            case GfxBufferUnorderedAccessMode::Disabled:
                throw GfxException("Buffer UAV is disabled");
            default:
                throw GfxException("Invalid buffer unordered access mode");
            }

            ID3D12Resource* pCounterResource;

            if (m_Desc.UnorderedAccessMode == GfxBufferUnorderedAccessMode::StructuredWithCounter)
            {
                pCounterResource = m_CounterResource->GetD3DResource();
            }
            else
            {
                pCounterResource = nullptr;
            }

            GfxDevice* device = GetDevice();
            m_UavDescriptor = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            device->GetD3DDevice4()->CreateUnorderedAccessView(m_DataResource->GetD3DResource(), pCounterResource, &desc, m_UavDescriptor.GetHandle());
        }

        return m_UavDescriptor.GetHandle();
    }

    D3D12_VERTEX_BUFFER_VIEW GfxBufferResource::GetVbv() const
    {
        D3D12_VERTEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(GfxBufferElement::Data);
        view.SizeInBytes = static_cast<UINT>(m_Desc.Count * m_Desc.Stride);
        view.StrideInBytes = static_cast<UINT>(m_Desc.Stride);
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW GfxBufferResource::GetIbv() const
    {
        D3D12_INDEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(GfxBufferElement::Data);
        view.SizeInBytes = static_cast<UINT>(m_Desc.Count * m_Desc.Stride);

        switch (m_Desc.Stride)
        {
        case 2:
            view.Format = DXGI_FORMAT_R16_UINT;
            break;
        case 4:
            view.Format = DXGI_FORMAT_R32_UINT;
            break;
        default:
            throw GfxException("Invalid index buffer stride");
        }

        return view;
    }

    RefCountPtr<GfxResource> GfxBufferDataDirectAllocator::Allocate(
        uint32_t sizeInBytes,
        uint32_t dataPlacementAlignment,
        uint32_t* pOutOffsetInBytes,
        GfxBufferDataAllocation* pOutAllocation)
    {
        *pOutOffsetInBytes = 0;

        GfxResourceAllocator* allocator = GetBaseAllocator();
        UINT64 width = static_cast<UINT64>(sizeInBytes);
        return allocator->Allocate("DirectBufferData", &CD3DX12_RESOURCE_DESC::Buffer(width), D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    GfxBuffer::GfxBuffer()
        : m_Resource{}
        , m_MappedData(nullptr)
    {
    }

    GfxBuffer::GfxBuffer(GfxDevice* device, const std::string& name, const GfxBufferDesc& desc, GfxAllocator allocator)
        : m_Resource{}
        , m_MappedData(nullptr)
    {
        GfxCompleteResourceAllocator* resAllocator = device->GetResourceAllocator(allocator, GfxAllocation::Buffer);
        UINT64 width = static_cast<UINT64>(desc.SizeInBytes);
        D3D12_RESOURCE_FLAGS flags = desc.UnorderedAccess ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
        m_Resource = resAllocator->Allocate(name, &CD3DX12_RESOURCE_DESC::Buffer(width, flags), desc.InitialState);

        TryMapData(resAllocator);
    }

    GfxBuffer::GfxBuffer(GfxDevice* device, uint32_t sizeInBytes, uint32_t dataPlacementAlignment, GfxSubAllocator allocator)
        : m_Resource{}
        , m_MappedData(nullptr)
    {
        GfxBufferSubAllocator* subAllocator = device->GetResourceAllocator(allocator);
        m_Resource = subAllocator->Allocate(sizeInBytes, dataPlacementAlignment);

        TryMapData(subAllocator);
    }

    GfxBuffer::GfxBuffer(GfxBuffer&& other) noexcept
        : m_Resource(std::move(other.m_Resource))
        , m_MappedData(std::exchange(other.m_MappedData, nullptr))
    {
    }

    GfxBuffer& GfxBuffer::operator=(GfxBuffer&& other)
    {
        if (this != &other)
        {
            Release();

            m_Resource = std::move(other.m_Resource);
            m_MappedData = std::exchange(other.m_MappedData, nullptr);
        }

        return *this;
    }

    void GfxBuffer::Release()
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
            memcpy(m_MappedData + m_Resource.GetBufferOffset() + destOffset, src, static_cast<size_t>(sizeInBytes));
        }
        else
        {
            GfxDevice* device = GetDevice();
            D3D12_RESOURCE_STATES currentState = GetResource()->GetState();

            GfxBuffer tempUpload{ device, sizeInBytes, 0, GfxSubAllocator::TempUpload };
            tempUpload.SetData(0, src, sizeInBytes);

            GfxCommandContext* context = device->RequestContext(GfxCommandType::Direct);
            context->CopyBuffer(&tempUpload, 0, this, destOffset, sizeInBytes);
            context->TransitionResource(GetResource().Get(), currentState);
            context->SubmitAndRelease().WaitOnCpu();
        }
    }
}
