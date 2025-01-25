#include "pch.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxCommand.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/MathUtils.h"
#include "Engine/Debug.h"
#include <stdexcept>

namespace march
{
    bool GfxBufferDesc::HasAllUsages(GfxBufferUsages usages) const
    {
        return (Usages & usages) == usages;
    }

    bool GfxBufferDesc::HasAnyUsages(GfxBufferUsages usages) const
    {
        return (Usages & usages) != static_cast<GfxBufferUsages>(0);
    }

    bool GfxBufferDesc::HasAllFlags(GfxBufferFlags flags) const
    {
        return (Flags & flags) == flags;
    }

    bool GfxBufferDesc::HasAnyFlags(GfxBufferFlags flags) const
    {
        return (Flags & flags) != GfxBufferFlags::None;
    }

    bool GfxBufferDesc::HasCounter() const
    {
        constexpr GfxBufferUsages usages
            = GfxBufferUsages::RWStructuredWithCounter
            | GfxBufferUsages::AppendStructured
            | GfxBufferUsages::ConsumeStructured;
        return HasAnyUsages(usages);
    }

    bool GfxBufferDesc::AllowUnorderedAccess() const
    {
        constexpr GfxBufferUsages usages
            = GfxBufferUsages::RWStructured
            | GfxBufferUsages::RWStructuredWithCounter
            | GfxBufferUsages::AppendStructured
            | GfxBufferUsages::ConsumeStructured
            | GfxBufferUsages::RWByteAddress;
        return HasAnyUsages(usages);
    }

    bool GfxBufferDesc::AllowUnorderedAccess(GfxBufferElement element) const
    {
        GfxBufferUsages usages;

        switch (element)
        {
        case GfxBufferElement::StructuredData:
            usages
                = GfxBufferUsages::RWStructured
                | GfxBufferUsages::RWStructuredWithCounter
                | GfxBufferUsages::AppendStructured
                | GfxBufferUsages::ConsumeStructured;
            break;
        case GfxBufferElement::RawData:
            usages
                = GfxBufferUsages::RWByteAddress;
            break;
        case GfxBufferElement::StructuredCounter:
        case GfxBufferElement::RawCounter:
            usages
                = GfxBufferUsages::RWStructuredWithCounter
                | GfxBufferUsages::AppendStructured
                | GfxBufferUsages::ConsumeStructured;
            break;
        default:
            throw GfxException("Invalid buffer element");
        }

        return HasAnyUsages(usages);
    }

    uint32_t GfxBufferDesc::GetSizeInBytes(GfxBufferElement element) const
    {
        switch (element)
        {
        case GfxBufferElement::StructuredData:
        case GfxBufferElement::RawData:
            return Stride * Count;
        case GfxBufferElement::StructuredCounter:
        case GfxBufferElement::RawCounter:
            return HasCounter() ? sizeof(uint32_t) : 0;
        default:
            throw GfxException("Invalid buffer element");
        }
    }

    bool GfxBufferDesc::IsCompatibleWith(const GfxBufferDesc& other) const
    {
        return Stride == other.Stride
            && Count >= other.Count
            && HasAllUsages(other.Usages)
            && Flags == other.Flags;
    }

    GfxBuffer::GfxBuffer(GfxDevice* device, const std::string& name) : GfxBuffer(device, name, GfxBufferDesc{}) {}

    GfxBuffer::GfxBuffer(GfxDevice* device, const std::string& name, const GfxBufferDesc& desc)
        : m_Device(device)
        , m_Name(name)
        , m_Desc(desc)
        , m_Resource(nullptr)
        , m_DataOffsetInBytes(0)
        , m_CounterOffsetInBytes(0)
        , m_Allocator(nullptr)
        , m_Allocation{}
        , m_UavDescriptors{}
    {
    }

    uint32_t GfxBuffer::GetOffsetInBytes(GfxBufferElement element)
    {
        AllocateResourceIfNot();

        switch (element)
        {
        case GfxBufferElement::StructuredData:
        case GfxBufferElement::RawData:
            return m_DataOffsetInBytes;
        case GfxBufferElement::StructuredCounter:
        case GfxBufferElement::RawCounter:
            if (!m_Desc.HasCounter())
            {
                throw GfxException("Buffer does not have counter");
            }
            return m_CounterOffsetInBytes;
        default:
            throw GfxException("Invalid buffer element");
        }
    }

    uint32_t GfxBuffer::GetSizeInBytes(GfxBufferElement element) const
    {
        return m_Desc.GetSizeInBytes(element);
    }

    RefCountPtr<GfxResource> GfxBuffer::GetUnderlyingResource()
    {
        AllocateResourceIfNot();
        return m_Resource;
    }

    ID3D12Resource* GfxBuffer::GetUnderlyingD3DResource()
    {
        return GetUnderlyingResource()->GetD3DResource();
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxBuffer::GetGpuVirtualAddress(GfxBufferElement element)
    {
        D3D12_GPU_VIRTUAL_ADDRESS baseAddress = GetUnderlyingD3DResource()->GetGPUVirtualAddress();
        return baseAddress + static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(GetOffsetInBytes(element));
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxBuffer::GetUav(GfxBufferElement element)
    {
        if (!m_Desc.AllowUnorderedAccess(element))
        {
            throw GfxException("Buffer element does not allow unordered access");
        }

        AllocateResourceIfNot();
        GfxOfflineDescriptor& uav = m_UavDescriptors[static_cast<size_t>(element)];

        if (!uav)
        {
            bool bindCounter = m_Desc.HasCounter();

            D3D12_UNORDERED_ACCESS_VIEW_DESC desc{};
            desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

            switch (element)
            {
            case GfxBufferElement::StructuredData:
                desc.Format = DXGI_FORMAT_UNKNOWN;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataOffsetInBytes / m_Desc.Stride);
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.Count);
                desc.Buffer.StructureByteStride = static_cast<UINT>(m_Desc.Stride);
                desc.Buffer.CounterOffsetInBytes = bindCounter ? m_CounterOffsetInBytes : 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                break;

            case GfxBufferElement::RawData:
                bindCounter = false; // 不支持 Counter
                desc.Format = DXGI_FORMAT_R32_TYPELESS;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_DataOffsetInBytes);
                desc.Buffer.NumElements = static_cast<UINT>(m_Desc.GetSizeInBytes(element));
                desc.Buffer.StructureByteStride = 0;
                desc.Buffer.CounterOffsetInBytes = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                break;

            case GfxBufferElement::StructuredCounter:
                if (!bindCounter) throw GfxException("Buffer does not have counter");
                bindCounter = false;
                desc.Format = DXGI_FORMAT_UNKNOWN;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_CounterOffsetInBytes / sizeof(uint32_t));
                desc.Buffer.NumElements = 1;
                desc.Buffer.StructureByteStride = sizeof(uint32_t);
                desc.Buffer.CounterOffsetInBytes = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
                break;

            case GfxBufferElement::RawCounter:
                if (!bindCounter) throw GfxException("Buffer does not have counter");
                bindCounter = false;
                desc.Format = DXGI_FORMAT_R32_TYPELESS;
                desc.Buffer.FirstElement = static_cast<UINT64>(m_CounterOffsetInBytes);
                desc.Buffer.NumElements = sizeof(uint32_t);
                desc.Buffer.StructureByteStride = 0;
                desc.Buffer.CounterOffsetInBytes = 0;
                desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                break;

            default:
                throw GfxException("Invalid buffer element");
            }

            GfxDevice* device = GetDevice();
            ID3D12Resource* pCounterResource = bindCounter ? m_Resource->GetD3DResource() : nullptr;
            uav = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            device->GetD3DDevice4()->CreateUnorderedAccessView(m_Resource->GetD3DResource(), pCounterResource, &desc, uav.GetHandle());
        }

        return uav.GetHandle();
    }

    D3D12_VERTEX_BUFFER_VIEW GfxBuffer::GetVbv()
    {
        if (!m_Desc.HasAllUsages(GfxBufferUsages::Vertex))
        {
            throw GfxException("Buffer can not be used as a vertex buffer");
        }

        D3D12_VERTEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(GfxBufferElement::StructuredData);
        view.SizeInBytes = static_cast<UINT>(m_Desc.GetSizeInBytes(GfxBufferElement::StructuredData));
        view.StrideInBytes = static_cast<UINT>(m_Desc.Stride);
        return view;
    }

    D3D12_INDEX_BUFFER_VIEW GfxBuffer::GetIbv()
    {
        if (!m_Desc.HasAllUsages(GfxBufferUsages::Index))
        {
            throw GfxException("Buffer can not be used as an index buffer");
        }

        D3D12_INDEX_BUFFER_VIEW view{};
        view.BufferLocation = GetGpuVirtualAddress(GfxBufferElement::StructuredData);
        view.SizeInBytes = static_cast<UINT>(m_Desc.GetSizeInBytes(GfxBufferElement::StructuredData));

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

    void GfxBuffer::SetData(const void* pData, std::optional<uint32_t> counter)
    {
        D3D12_RANGE resourceRange = ReallocateResource();

        if (!pData && !counter)
        {
            return;
        }

        if (m_Resource->IsHeapCpuAccessible())
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-unmap

            uint8_t* pMappedData = nullptr;
            ID3D12Resource* d3dResource = m_Resource->GetD3DResource();
            GFX_HR(d3dResource->Map(0, &CD3DX12_RANGE(0, 0), reinterpret_cast<void**>(&pMappedData))); // Write-Only

            if (pData)
            {
                size_t offset = static_cast<size_t>(m_DataOffsetInBytes);
                size_t size = static_cast<size_t>(m_Desc.GetSizeInBytes(GfxBufferElement::StructuredData));
                memcpy(pMappedData + offset, pData, size);
            }

            if (counter)
            {
                if (m_Desc.HasCounter())
                {
                    uint32_t value = *counter;
                    assert(sizeof(value) == m_Desc.GetSizeInBytes(GfxBufferElement::StructuredCounter));

                    size_t offset = static_cast<size_t>(m_CounterOffsetInBytes);
                    memcpy(pMappedData + offset, &value, sizeof(value));
                }
                else
                {
                    LOG_WARNING("GfxBuffer::SetData: buffer does not have counter");
                }
            }

            d3dResource->Unmap(0, &resourceRange);
        }
        else
        {
            GfxCommandContext* context = m_Device->RequestContext(GfxCommandType::Direct);

            if (pData)
            {
                GfxBufferDesc tempDesc{};
                tempDesc.Stride = m_Desc.GetSizeInBytes(GfxBufferElement::StructuredData);
                tempDesc.Count = 1;
                tempDesc.Usages = GfxBufferUsages::Copy;
                tempDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

                GfxBuffer temp{ m_Device, m_Name + "DataTempUpload", tempDesc };
                temp.SetData(pData);
                context->CopyBuffer(&temp, GfxBufferElement::StructuredData, this, GfxBufferElement::StructuredData);
            }

            if (counter)
            {
                if (m_Desc.HasCounter())
                {
                    GfxBufferDesc tempDesc{};
                    tempDesc.Stride = m_Desc.GetSizeInBytes(GfxBufferElement::StructuredCounter);
                    tempDesc.Count = 1;
                    tempDesc.Usages = GfxBufferUsages::Copy;
                    tempDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

                    uint32_t value = *counter;
                    assert(sizeof(value) == tempDesc.Stride);

                    GfxBuffer temp{ m_Device, m_Name + "CounterTempUpload", tempDesc };
                    temp.SetData(&value);
                    context->CopyBuffer(&temp, GfxBufferElement::StructuredData, this, GfxBufferElement::StructuredCounter);
                }
                else
                {
                    LOG_WARNING("GfxBuffer::SetData: buffer does not have counter");
                }
            }

            context->SubmitAndRelease().WaitOnCpu();
        }
    }

    void GfxBuffer::SetData(const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter)
    {
        m_Desc = desc;
        SetData(pData, counter);
    }

    void GfxBuffer::ReleaseResource()
    {
        if (m_Resource)
        {
            m_Device->DeferredRelease(m_Resource);
            m_Resource = nullptr;
        }

        if (m_Allocator)
        {
            m_Allocator->DeferredRelease(m_Allocation);
            m_Allocator = nullptr;
        }

        for (GfxOfflineDescriptor& uav : m_UavDescriptors)
        {
            uav.DeferredRelease();
        }
    }

    void GfxBuffer::AllocateResourceIfNot()
    {
        if (!m_Resource)
        {
            ReallocateResource();
        }
    }

    D3D12_RANGE GfxBuffer::ReallocateResource()
    {
        ReleaseResource();

        uint32_t sizeInBytes = m_Desc.GetSizeInBytes(GfxBufferElement::StructuredData);
        uint32_t dataPlacementAlignment = 0; // TODO: 不知道默认能不能用 0，或许要用 16 字节（float4）？

        if (m_Desc.HasAllUsages(GfxBufferUsages::Index) && m_Desc.Stride != 2 && m_Desc.Stride != 4)
        {
            throw std::invalid_argument("GfxBuffer::AllocateResource: stride must be 2 or 4 bytes for Index Buffer");
        }

        if (m_Desc.HasAllUsages(GfxBufferUsages::Constant))
        {
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
        }

        if (m_Desc.AllowUnorderedAccess(GfxBufferElement::StructuredData))
        {
            // 创建 UAV 时需要填 FirstElement，所以 Offset 必须是 Stride 的整数倍
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, m_Desc.Stride);
        }

        uint32_t dataOffsetInResource = 0;

        if (m_Desc.HasCounter())
        {
            // 在 Data 前面放 4 字节的 Counter
            // 布局：Counter [Padding] Data
            // Padding 用于对齐 Data，可能没有

            dataOffsetInResource = MathUtils::AlignUp(4, dataPlacementAlignment);
            sizeInBytes += dataOffsetInResource;
            dataPlacementAlignment = std::max<uint32_t>(dataPlacementAlignment, D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT);
        }

        bool isSubAlloc;

        if (m_Desc.AllowUnorderedAccess())
        {
            isSubAlloc = false;

            if (m_Desc.HasAnyFlags(GfxBufferFlags::Transient))
            {
                LOG_WARNING("GfxBuffer::AllocateResource: The Transient flag will be ignored because the buffer allows unordered access");
            }
        }
        else
        {
            // 尽可能使用 SubAlloc，优化性能
            isSubAlloc = m_Desc.HasAnyFlags(GfxBufferFlags::Dynamic | GfxBufferFlags::Transient);
        }

        uint32_t resourceOffsetInBytes = 0;

        if (isSubAlloc)
        {
            bool isFastOneFrame = m_Desc.HasAnyFlags(GfxBufferFlags::Transient);
            m_Allocator = m_Device->GetUploadHeapBufferSubAllocator(isFastOneFrame);
            m_Resource = m_Allocator->Allocate(sizeInBytes, dataPlacementAlignment, &resourceOffsetInBytes, &m_Allocation);
        }
        else
        {
            GfxResourceAllocator* allocator = m_Device->GetPlacedBufferAllocator(
                m_Desc.HasAnyFlags(GfxBufferFlags::Dynamic) ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT);

            UINT64 width = static_cast<UINT64>(sizeInBytes);
            D3D12_RESOURCE_FLAGS flags = m_Desc.AllowUnorderedAccess() ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE;
            m_Resource = allocator->Allocate(m_Name, &CD3DX12_RESOURCE_DESC::Buffer(width, flags), D3D12_RESOURCE_STATE_GENERIC_READ);
        }

        m_DataOffsetInBytes = resourceOffsetInBytes + dataOffsetInResource;
        m_CounterOffsetInBytes = resourceOffsetInBytes;
        return CD3DX12_RANGE(resourceOffsetInBytes, resourceOffsetInBytes + sizeInBytes);
    }

    GfxBuffer::GfxBuffer(GfxBuffer&& other)
        : m_Device(other.m_Device)
        , m_Name(std::move(other.m_Name))
        , m_Desc(other.m_Desc)
        , m_Resource(std::move(other.m_Resource))
        , m_DataOffsetInBytes(other.m_DataOffsetInBytes)
        , m_CounterOffsetInBytes(other.m_CounterOffsetInBytes)
        , m_Allocator(std::exchange(other.m_Allocator, nullptr))
        , m_Allocation(other.m_Allocation)
        , m_UavDescriptors{}
    {
        for (size_t i = 0; i < std::size(m_UavDescriptors); i++)
        {
            m_UavDescriptors[i] = std::move(other.m_UavDescriptors[i]);
        }
    }

    GfxBuffer& GfxBuffer::operator=(GfxBuffer&& other)
    {
        if (this != &other)
        {
            ReleaseResource();

            m_Device = other.m_Device;
            m_Name = std::move(other.m_Name);
            m_Desc = other.m_Desc;
            m_Resource = std::move(other.m_Resource);
            m_DataOffsetInBytes = other.m_DataOffsetInBytes;
            m_CounterOffsetInBytes = other.m_CounterOffsetInBytes;
            m_Allocator = std::exchange(other.m_Allocator, nullptr);
            m_Allocation = other.m_Allocation;

            for (size_t i = 0; i < std::size(m_UavDescriptors); i++)
            {
                m_UavDescriptors[i] = std::move(other.m_UavDescriptors[i]);
            }
        }

        return *this;
    }

    GfxBufferMultiBuddySubAllocator::GfxBufferMultiBuddySubAllocator(
        const std::string& name,
        const GfxBufferMultiBuddySubAllocatorDesc& desc,
        GfxResourceAllocator* pageAllocator)
        : m_Device(pageAllocator->GetDevice())
        , m_Pages{}
        , m_ReleaseQueue{}
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

    void GfxBufferMultiBuddySubAllocator::DeferredRelease(const GfxBufferSubAllocation& allocation)
    {
        m_ReleaseQueue.emplace(m_Device->GetNextFence(), allocation);
    }

    void GfxBufferMultiBuddySubAllocator::CleanUpAllocations()
    {
        while (!m_ReleaseQueue.empty() && m_Device->IsFenceCompleted(m_ReleaseQueue.front().first))
        {
            m_Allocator->Release(m_ReleaseQueue.front().second.Buddy);
            m_ReleaseQueue.pop();
        }
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

    void GfxBufferLinearSubAllocator::DeferredRelease(const GfxBufferSubAllocation& allocation)
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
}
