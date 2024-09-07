#include <directx/d3dx12.h>
#include "Rendering/DescriptorHeap.h"
#include "Rendering/GfxManager.h"
#include "Rendering/DxException.h"
#include "Core/Debug.h"

namespace dx12demo
{
    namespace
    {
        LPCWSTR GetDescriptorHeapTypeName(D3D12_DESCRIPTOR_HEAP_TYPE type)
        {
            switch (type)
            {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
                return L"CBV_SRV_UAV";
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
                return L"SAMPLER";
            case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
                return L"RTV";
            case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
                return L"DSV";
            default:
                return L"UNKNOWN";
            }
        }
    }

    DescriptorHeap::DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT fixedCapacity, UINT dynamicCapacity, bool shaderVisible)
        : m_DynamicBase(0)
        , m_DynamicRear(0)
        , m_DynamicFront(0)
        , m_DynamicCapacity(dynamicCapacity)
        , m_DynamicUsed{}
        , m_FixedCapacity(fixedCapacity)
    {
        auto device = GetGfxManager().GetDevice();
        m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = dynamicCapacity + fixedCapacity;
        desc.Type = type;
        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

#if defined(ENABLE_GFX_DEBUG_NAME)
        THROW_IF_FAILED(m_Heap->SetName(name.c_str()));
#else
        (name);
#endif
    }

    DescriptorHeap::~DescriptorHeap()
    {
        GetGfxManager().SafeReleaseObject(m_Heap);
        m_Heap = nullptr;
    }

    UINT DescriptorHeap::Append()
    {
        if (m_DynamicCapacity <= 0)
        {
            throw std::out_of_range("The capacity of dynamic descriptor heap is zero");
        }

        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();

        // 清理已经使用完成的空间
        while (!m_DynamicUsed.empty() && m_DynamicUsed.front().first <= completedFenceValue)
        {
            m_DynamicFront = m_DynamicUsed.front().second;
            m_DynamicUsed.pop();
        }

        UINT newRear = (m_DynamicRear + 1) % m_DynamicCapacity;

        if (newRear == m_DynamicFront)
        {
            throw std::out_of_range("Dynamic descriptor heap is full");
        }

        return std::exchange(m_DynamicRear, newRear) - m_DynamicBase;
    }

    void DescriptorHeap::Flush()
    {
        UINT64 fenceValue = GetGfxManager().GetNextFenceValue();
        m_DynamicUsed.emplace(fenceValue, m_DynamicRear);
        m_DynamicBase = m_DynamicRear; // 更新起点
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(DescriptorHeapRegion region, UINT index) const
    {
        if (index < 0 || index >= GetCount(region))
        {
            throw std::out_of_range("Index out of the range of descriptor heap");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());

        switch (region)
        {
        case dx12demo::DescriptorHeapRegion::Fixed:
            return handle.Offset(m_DynamicCapacity + index, m_DescriptorSize);
        case dx12demo::DescriptorHeapRegion::Dynamic:
            return handle.Offset((m_DynamicBase + index) % m_DynamicCapacity, m_DescriptorSize);
        default:
            THROW_INVALID_DESCRIPTOR_HEAP_REGION;
        }
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(DescriptorHeapRegion region, UINT index) const
    {
        if (index < 0 || index >= GetCount(region))
        {
            throw std::out_of_range("Index out of the range of descriptor heap");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_Heap->GetGPUDescriptorHandleForHeapStart());

        switch (region)
        {
        case dx12demo::DescriptorHeapRegion::Fixed:
            return handle.Offset(m_DynamicCapacity + index, m_DescriptorSize);
        case dx12demo::DescriptorHeapRegion::Dynamic:
            return handle.Offset((m_DynamicBase + index) % m_DynamicCapacity, m_DescriptorSize);
        default:
            THROW_INVALID_DESCRIPTOR_HEAP_REGION;
        }
    }

    void DescriptorHeap::Copy(DescriptorHeapRegion region, UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        auto device = GetGfxManager().GetDevice();
        D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor = GetCpuHandle(region, destIndex);
        device->CopyDescriptorsSimple(1, destDescriptor, srcDescriptor, GetHeapType());
    }

    DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, UINT pageSize)
        : m_DescriptorType(descriptorType), m_PageSize(pageSize)
    {

    }

    DescriptorHandle DescriptorAllocator::Allocate()
    {
        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();

        if (!m_FreeList.empty() && m_FreeList.front().first <= completedFenceValue)
        {
            DescriptorHandle result = m_FreeList.front().second;
            m_FreeList.pop();
            return result;
        }

        if (m_Pages.empty() || m_Pages.back()->IsFull(DescriptorHeapRegion::Dynamic))
        {
            std::wstring name = L"DescriptorAllocatorPage" + std::to_wstring(m_Pages.size());
            m_Pages.push_back(std::make_unique<DescriptorHeap>(name, m_DescriptorType, 0, m_PageSize, false));
            DEBUG_LOG_INFO(L"Create %s; Size: %d; Type: %s", name.c_str(), m_PageSize, GetDescriptorHeapTypeName(m_DescriptorType));
        }

        return std::make_pair(m_Pages.size() - 1, m_Pages.back()->Append());
    }

    void DescriptorAllocator::Free(const DescriptorHandle& handle)
    {
        m_FreeList.emplace(GetGfxManager().GetNextFenceValue(), handle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetCpuHandle(const DescriptorHandle& handle) const
    {
        return m_Pages[handle.first]->GetCpuHandle(DescriptorHeapRegion::Dynamic, handle.second);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorAllocator::GetGpuHandle(const DescriptorHandle& handle) const
    {
        return m_Pages[handle.first]->GetGpuHandle(DescriptorHeapRegion::Dynamic, handle.second);
    }

    TypedDescriptorHandle DescriptorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType)
    {
        return std::make_pair(descriptorType, GetAllocator(descriptorType)->Allocate());
    }

    void DescriptorManager::Free(const TypedDescriptorHandle& handle)
    {
        GetAllocator(handle.first)->Free(handle.second);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetCpuHandle(const TypedDescriptorHandle& handle)
    {
        return GetAllocator(handle.first)->GetCpuHandle(handle.second);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetGpuHandle(const TypedDescriptorHandle& handle)
    {
        return GetAllocator(handle.first)->GetGpuHandle(handle.second);
    }

    std::unique_ptr<DescriptorAllocator> DescriptorManager::s_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

    DescriptorAllocator* DescriptorManager::GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType)
    {
        std::unique_ptr<DescriptorAllocator>& allocator = s_Allocators[descriptorType];

        if (allocator == nullptr)
        {
            allocator = std::make_unique<DescriptorAllocator>(descriptorType, AllocatorPageSize);
        }

        return allocator.get();
    }

    DescriptorHeapSpan::DescriptorHeapSpan(DescriptorHeap* heap, UINT offset, UINT count)
        : m_Heap(heap), m_Offset(offset), m_Count(count)
    {

    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapSpan::GetCpuHandle(UINT index) const
    {
        if (index < 0 || index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        return m_Heap->GetCpuHandle(DescriptorHeapRegion::Fixed, m_Offset + index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapSpan::GetGpuHandle(UINT index) const
    {
        if (index < 0 || index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        return m_Heap->GetGpuHandle(DescriptorHeapRegion::Fixed, m_Offset + index);
    }

    void DescriptorHeapSpan::Copy(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        if (destIndex < 0 || destIndex >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        m_Heap->Copy(DescriptorHeapRegion::Fixed, m_Offset + destIndex, srcDescriptor);
    }

    DescriptorHeapAllocator::DescriptorHeapAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT pageSize)
        : m_HeapType(heapType), m_PageSize(pageSize)
    {

    }

    DescriptorHeapSpan DescriptorHeapAllocator::Allocate(UINT count)
    {
        if (count > m_PageSize)
        {
            throw std::out_of_range("Requested descriptor count exceeds the page size of DescriptorHeapAllocator");
        }

        if (m_ActiveHeaps.empty() || m_ActiveHeaps.back()->GetCapacity(DescriptorHeapRegion::Fixed) - m_Offset < count)
        {
            m_ActiveHeaps.push_back(RequestNewHeap());
            m_Offset = 0;
        }

        DescriptorHeapSpan span(m_ActiveHeaps.back(), m_Offset, count);
        m_Offset += count;
        return span;
    }

    void DescriptorHeapAllocator::Flush(UINT64 fenceValue)
    {
        for (DescriptorHeap* heap : m_ActiveHeaps)
        {
            m_PendingHeaps.emplace(fenceValue, heap);
        }

        m_ActiveHeaps.clear();
        m_Offset = 0;
    }

    DescriptorHeap* DescriptorHeapAllocator::RequestNewHeap()
    {
        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();

        if (!m_PendingHeaps.empty() && m_PendingHeaps.front().first <= completedFenceValue)
        {
            DescriptorHeap* heap = m_PendingHeaps.front().second;
            m_PendingHeaps.pop();
            return heap;
        }

        std::wstring name = L"DescriptorHeapAllocatorPage" + std::to_wstring(m_AllHeaps.size());
        m_AllHeaps.push_back(std::make_unique<DescriptorHeap>(name.c_str(), m_HeapType, m_PageSize, 0, true));
        DEBUG_LOG_INFO(L"Create %s; Size: %d; Type: %s", name.c_str(), m_PageSize, GetDescriptorHeapTypeName(m_HeapType));
        return m_AllHeaps.back().get();
    }
}
