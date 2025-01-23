#include "pch.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxCommand.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/MathUtils.h"
#include "Engine/Debug.h"
#include <stdexcept>

namespace march
{
    GfxBufferResource::GfxBufferResource(
        const GfxBufferDesc& desc,
        RefCountPtr<GfxResource> resource,
        uint32_t dataOffsetInBytes,
        uint32_t counterOffsetInBytes)
        : GfxBufferResource(desc, nullptr, GfxBufferSubAllocation{}, resource, dataOffsetInBytes, counterOffsetInBytes)
    {
    }

    GfxBufferResource::GfxBufferResource(
        const GfxBufferDesc& desc,
        GfxBufferSubAllocator* allocator,
        const GfxBufferSubAllocation& allocation,
        RefCountPtr<GfxResource> resource,
        uint32_t dataOffsetInBytes,
        uint32_t counterOffsetInBytes)
        : m_Desc(desc)
        , m_Resource(resource)
        , m_DataOffsetInBytes(dataOffsetInBytes)
        , m_CounterOffsetInBytes(counterOffsetInBytes)
        , m_Allocator(allocator)
        , m_Allocation(allocation)
        , m_UavDescriptor{}
    {
    }

    GfxBufferResource::~GfxBufferResource()
    {
        if (m_Allocator)
        {
            m_Allocator->Release(m_Allocation);
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxBufferResource::GetGpuVirtualAddress(GfxBufferElement element) const
    {
        D3D12_GPU_VIRTUAL_ADDRESS baseAddress = m_Resource->GetD3DResource()->GetGPUVirtualAddress();
        return baseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(GetOffsetInBytes(element));
    }

    uint32_t GfxBufferResource::GetOffsetInBytes(GfxBufferElement element) const
    {
        switch (element)
        {
        case GfxBufferElement::Data:
            return m_DataOffsetInBytes;

        case GfxBufferElement::Counter:
            if (!m_Desc.HasCounter())
            {
                throw GfxException("Buffer does not have counter");
            }
            return m_CounterOffsetInBytes;

        default:
            throw GfxException("Invalid buffer element");
        }
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
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataOffsetInBytes / m_Desc.Stride);
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.Count);
                desc.Buffer.StructureByteStride = static_cast<UINT>(m_Desc.Stride);
                desc.Buffer.CounterOffsetInBytes = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                break;
            case GfxBufferUnorderedAccessMode::StructuredWithCounter:
                desc.Format = DXGI_FORMAT_UNKNOWN;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataOffsetInBytes / m_Desc.Stride);
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.Count);
                desc.Buffer.StructureByteStride = static_cast<UINT>(m_Desc.Stride);
                desc.Buffer.CounterOffsetInBytes = m_CounterOffsetInBytes;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                break;
            case GfxBufferUnorderedAccessMode::Raw:
                desc.Format = DXGI_FORMAT_R32_TYPELESS;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataOffsetInBytes / sizeof(uint32_t));
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.GetSizeInBytes() / sizeof(uint32_t));
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

            if (m_Desc.HasCounter())
            {
                pCounterResource = m_Resource->GetD3DResource();
            }
            else
            {
                pCounterResource = nullptr;
            }

            GfxDevice* device = GetDevice();
            m_UavDescriptor = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            device->GetD3DDevice4()->CreateUnorderedAccessView(m_Resource->GetD3DResource(), pCounterResource, &desc, m_UavDescriptor.GetHandle());
        }

        return m_UavDescriptor.GetHandle();
    }

    D3D12_VERTEX_BUFFER_VIEW GfxBufferResource::GetVbv() const
    {
        D3D12_VERTEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(GfxBufferElement::Data);
        view.SizeInBytes = static_cast<UINT>(m_Desc.GetSizeInBytes());
        view.StrideInBytes = static_cast<UINT>(m_Desc.Stride);
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW GfxBufferResource::GetIbv() const
    {
        D3D12_INDEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(GfxBufferElement::Data);
        view.SizeInBytes = static_cast<UINT>(m_Desc.GetSizeInBytes());

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

    GfxBufferMultiBuddySubAllocator::GfxBufferMultiBuddySubAllocator(
        const std::string& name,
        const GfxBufferMultiBuddySubAllocatorDesc& desc,
        GfxResourceAllocator* pageAllocator)
        : m_Pages{}
    {
        auto appendPageFunc = [this, pageAllocator](uint32_t sizeInBytes)
        {
            UINT64 pageWidth = static_cast<UINT64>(sizeInBytes);
            std::string pageName = m_Allocator->GetName() + "Page";
            D3D12_RESOURCE_STATES pageState = D3D12_RESOURCE_STATE_GENERIC_READ;
            m_Pages.emplace_back(pageAllocator->Allocate(pageName, &CD3DX12_RESOURCE_DESC::Buffer(pageWidth), pageState));
        };

        m_Allocator = std::make_unique<MultiBuddyAllocator>(name, desc.MinBlockSize, desc.DefaultMaxBlockSize, appendPageFunc);
    }

    RefCountPtr<GfxResource> GfxBufferMultiBuddySubAllocator::Allocate(
        uint32_t sizeInBytes,
        uint32_t dataPlacementAlignment,
        uint32_t* pOutOffsetInBytes,
        GfxBufferSubAllocation* pOutAllocation)
    {
        size_t pageIndex = 0;

        if (std::optional<uint32_t> offset = m_Allocator->Allocate(sizeInBytes, dataPlacementAlignment, &pageIndex, &pOutAllocation->Buddy))
        {
            *pOutOffsetInBytes = *offset;
            return m_Pages[pageIndex];
        }

        return nullptr;
    }

    void GfxBufferMultiBuddySubAllocator::Release(const GfxBufferSubAllocation& allocation)
    {
        m_Allocator->Release(allocation.Buddy);
    }

    GfxBufferLinearSubAllocator::GfxBufferLinearSubAllocator(
        const std::string& name,
        const GfxBufferLinearSubAllocatorDesc& desc,
        GfxResourceAllocator* pageAllocator,
        GfxResourceAllocator* largePageAllocator)
        : m_Device(pageAllocator->GetDevice())
        , m_Pages{}
        , m_LargePages{}
        , m_ReleaseQueue{}
    {
        auto requestPageFunc = [this, pageAllocator, largePageAllocator](uint32_t sizeInBytes, bool large, bool* pOutIsNew) -> size_t
        {
            std::vector<RefCountPtr<GfxResource>>& pages = large ? m_LargePages : m_Pages;

            if (!large && !m_ReleaseQueue.empty() && m_Device->IsFenceCompleted(m_ReleaseQueue.front().first))
            {
                *pOutIsNew = false;

                pages.emplace_back(std::move(m_ReleaseQueue.front().second));
                m_ReleaseQueue.pop();
            }
            else
            {
                *pOutIsNew = true;

                GfxResourceAllocator* allocator;
                std::string pageName;

                if (large)
                {
                    allocator = largePageAllocator;
                    pageName = m_Allocator->GetName() + "Page (Large)";
                }
                else
                {
                    allocator = pageAllocator;
                    pageName = m_Allocator->GetName() + "Page";
                }

                UINT64 pageWidth = static_cast<UINT64>(sizeInBytes);
                D3D12_RESOURCE_STATES pageState = D3D12_RESOURCE_STATE_GENERIC_READ;
                pages.emplace_back(allocator->Allocate(pageName, &CD3DX12_RESOURCE_DESC::Buffer(pageWidth), pageState));
            }

            return pages.size() - 1;
        };

        m_Allocator = std::make_unique<LinearAllocator>(name, desc.PageSize, requestPageFunc);
    }

    RefCountPtr<GfxResource> GfxBufferLinearSubAllocator::Allocate(
        uint32_t sizeInBytes,
        uint32_t dataPlacementAlignment,
        uint32_t* pOutOffsetInBytes,
        GfxBufferSubAllocation* pOutAllocation)
    {
        size_t pageIndex = 0;
        bool large = false;
        *pOutOffsetInBytes = m_Allocator->Allocate(sizeInBytes, dataPlacementAlignment, &pageIndex, &large);
        return (large ? m_LargePages : m_Pages)[pageIndex];
    }

    void GfxBufferLinearSubAllocator::Release(const GfxBufferSubAllocation& allocation)
    {
        // Do Nothing
    }

    void GfxBufferLinearSubAllocator::CleanUpAllocations()
    {
        uint64_t nextFence = m_Device->GetNextFence();

        for (RefCountPtr<GfxResource>& page : m_Pages)
        {
            m_ReleaseQueue.emplace(nextFence, std::move(page));
        }

        m_Allocator->Reset();
        m_Pages.clear();
        m_LargePages.clear();
    }

    void Release()
    {
        if (m_MappedData)
        {
            m_MappedData = nullptr;

            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-unmap
            GetResource()->GetD3DResource()->Unmap(0, nullptr);
        }
    }

    void TryMapData(GfxResourceAllocator* allocator)
    {
        if (allocator->IsHeapCpuAccessible())
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map
            GFX_HR(GetResource()->GetD3DResource()->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
        }
    }

    void GfxBuffer::SetData(
        const GfxBufferDesc& desc,
        GfxBufferAllocationStrategy allocationStrategy,
        const void* pData,
        uint32_t counter)
    {
        uint32_t totalSizeInBytes = AllocateResource(desc, allocationStrategy);

        if (!pData && counter == NullCounter)
        {
            return;
        }

        RefCountPtr<GfxResource> underlyingResource = m_Resource->GetUnderlyingResource();
        ID3D12Resource* d3dResource = underlyingResource->GetD3DResource();

        size_t dataOffsetInBytes = static_cast<size_t>(m_Resource->GetOffsetInBytes(GfxBufferElement::Data));
        size_t dataSizeInBytes = static_cast<size_t>(m_Resource->GetDesc().GetSizeInBytes());

        if (underlyingResource->IsHeapCpuAccessible())
        {
            uint8_t* pMappedData = nullptr;
            GFX_HR(d3dResource->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&pMappedData)));

            if (pData)
            {
                memcpy(pMappedData + dataOffsetInBytes, pData, dataSizeInBytes);
            }

            if (counter != NullCounter)
            {
                if (desc.HasCounter())
                {

                }
                else
                {
                    LOG_WARNING("GfxBuffer::SetData: buffer does not have counter");
                }
            }

            d3dResource->Unmap(0, &CD3DX12_RANGE(dataOffsetInBytes, dataOffsetInBytes + dataSizeInBytes));
        }
        else
        {
            GfxBufferDesc tempDesc{};
            tempDesc.Stride = totalSizeInBytes;
            tempDesc.Count = 1;
            tempDesc.Usages = GfxBufferUsages::Copy;
            tempDesc.UnorderedAccessMode = GfxBufferUnorderedAccessMode::Disabled;

            GfxBuffer temp{ m_Device, m_Name + "TempUpload" };
            temp.SetData(tempDesc, GfxBufferAllocationStrategy::UploadHeapFastOneFrame, pData, counter);

            //GfxDevice* device = GetDevice();
            //D3D12_RESOURCE_STATES currentState = GetResource()->GetState();

            //GfxBuffer tempUpload{ device, sizeInBytes, 0, GfxSubAllocator::TempUpload };
            //tempUpload.SetData(0, src, sizeInBytes);

            //GfxCommandContext* context = device->RequestContext(GfxCommandType::Direct);
            //context->CopyBuffer(&tempUpload, 0, this, destOffset, sizeInBytes);
            //context->TransitionResource(GetResource().Get(), currentState);
            //context->SubmitAndRelease().WaitOnCpu();
        }
    }

    uint32_t GfxBuffer::AllocateResource(const GfxBufferDesc& desc, GfxBufferAllocationStrategy strategy)
    {
        uint32_t sizeInBytes = desc.GetSizeInBytes();
        uint32_t dataPlacementAlignment = 0; // TODO: 不知道默认能不能用 0，或许要用 16 字节（float4）？

        if ((desc.Usages & GfxBufferUsages::Index) == GfxBufferUsages::Index && desc.Stride != 2 && desc.Stride != 4)
        {
            throw std::invalid_argument("GfxBuffer::AllocateResource: stride must be 2 or 4 bytes for Index Buffer");
        }

        if ((desc.Usages & GfxBufferUsages::Constant) == GfxBufferUsages::Constant)
        {
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        }

        if (desc.UnorderedAccessMode == GfxBufferUnorderedAccessMode::Structured || desc.UnorderedAccessMode == GfxBufferUnorderedAccessMode::StructuredWithCounter)
        {
            // 创建 UAV 时需要填 FirstElement，所以 Offset 必须是 Stride 的整数倍
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, desc.Stride);
        }
        else if (desc.UnorderedAccessMode == GfxBufferUnorderedAccessMode::Raw)
        {
            if (sizeInBytes % sizeof(uint32_t) != 0)
            {
                throw std::invalid_argument("GfxBuffer::AllocateResource: size must be a multiple of 4 bytes for Raw buffer");
            }

            // 创建 UAV 时需要填 FirstElement，所以 Offset 必须是 sizeof(uint32_t) 的整数倍
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, sizeof(uint32_t));
        }

        uint32_t dataOffsetInResource = 0;

        if (desc.HasCounter())
        {
            // 在 Data 前面放 4 字节的 Counter
            // 布局：Counter [Padding] Data
            // Padding 用于对齐 Data，可能没有

            dataOffsetInResource = MathUtils::AlignUp(4, dataPlacementAlignment);
            sizeInBytes += dataOffsetInResource;
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);
        }

        bool isSubAlloc = false;

        switch (strategy)
        {
        case GfxBufferAllocationStrategy::DefaultHeapCommitted:
        case GfxBufferAllocationStrategy::DefaultHeapPlaced:
        case GfxBufferAllocationStrategy::UploadHeapPlaced:
            isSubAlloc = false;
            break;
        case GfxBufferAllocationStrategy::UploadHeapSubAlloc:
        case GfxBufferAllocationStrategy::UploadHeapFastOneFrame:
            isSubAlloc = true;
            break;
        default:
            throw std::invalid_argument("GfxBuffer::AllocateResource: Invalid allocation strategy");
        }

        uint32_t resourceOffsetInBytes = 0;
        GfxBufferSubAllocator* subAllocator = nullptr;
        GfxBufferSubAllocation subAllocation{};
        RefCountPtr<GfxResource> resource;

        if (isSubAlloc)
        {
            if (desc.UnorderedAccessMode != GfxBufferUnorderedAccessMode::Disabled)
            {
                throw std::invalid_argument("GfxBuffer::AllocateResource: Unordered Access is not supported for sub-allocated buffer");
            }

            bool isFastOneFrame = (strategy == GfxBufferAllocationStrategy::UploadHeapFastOneFrame);
            subAllocator = m_Device->GetUploadHeapBufferSubAllocator(isFastOneFrame);
            resource = subAllocator->Allocate(sizeInBytes, dataPlacementAlignment, &resourceOffsetInBytes, &subAllocation);
        }
        else
        {
            GfxResourceAllocator* allocator;

            switch (strategy)
            {
            case GfxBufferAllocationStrategy::DefaultHeapCommitted:
                allocator = m_Device->GetCommittedAllocator(D3D12_HEAP_TYPE_DEFAULT);
                break;
            case GfxBufferAllocationStrategy::DefaultHeapPlaced:
                allocator = m_Device->GetPlacedBufferAllocator(D3D12_HEAP_TYPE_DEFAULT);
                break;
            case GfxBufferAllocationStrategy::UploadHeapPlaced:
                allocator = m_Device->GetPlacedBufferAllocator(D3D12_HEAP_TYPE_UPLOAD);
                break;
            default:
                throw std::invalid_argument("GfxBuffer::AllocateResource: Invalid allocation strategy");
            }

            UINT64 width = static_cast<UINT64>(sizeInBytes);
            D3D12_RESOURCE_FLAGS flags;

            if (desc.UnorderedAccessMode == GfxBufferUnorderedAccessMode::Disabled)
            {
                flags = D3D12_RESOURCE_FLAG_NONE;
            }
            else
            {
                flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }

            resource = allocator->Allocate(m_Name, &CD3DX12_RESOURCE_DESC::Buffer(width, flags), D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        m_Resource = MARCH_MAKE_REF(GfxBufferResource, desc, subAllocator, subAllocation, resource,
            /* dataOffsetInBytes */ resourceOffsetInBytes + dataOffsetInResource,
            /* counterOffsetInBytes */ resourceOffsetInBytes);
        return sizeInBytes;
    }
}
