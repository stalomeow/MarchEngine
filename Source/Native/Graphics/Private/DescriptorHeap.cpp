#include "DescriptorHeap.h"
#include "GfxManager.h"
#include "DxException.h"
#include "WinApplication.h"
#include "Debug.h"
#include <assert.h>

namespace march
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

    DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, bool shaderVisible, const std::wstring& name)
        : m_Device(device)
    {
        m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = capacity;
        desc.Type = type;
        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

#if defined(ENABLE_GFX_DEBUG_NAME)
        THROW_IF_FAILED(m_Heap->SetName(name.c_str()));
#else
        (name);
#endif
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandle(UINT index) const
    {
        if (index < 0 || index >= GetCapacity())
        {
            throw std::out_of_range("GetCpuHandle: Index out of the range of descriptor heap");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        return handle.Offset(index, m_DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT index) const
    {
        if (index < 0 || index >= GetCapacity())
        {
            throw std::out_of_range("GetGpuHandle: Index out of the range of descriptor heap");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_Heap->GetGPUDescriptorHandleForHeapStart());
        return handle.Offset(index, m_DescriptorSize);
    }

    void DescriptorHeap::Copy(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor = GetCpuHandle(destIndex);
        m_Device->CopyDescriptorsSimple(1, destDescriptor, srcDescriptor, GetType());
    }

    DescriptorAllocator::DescriptorAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, UINT pageSize)
        : m_Device(device), m_DescriptorType(descriptorType), m_PageSize(pageSize)
    {

    }

    DescriptorHandle DescriptorAllocator::Allocate(UINT64 completedFenceValue)
    {
        if (!m_FreeList.empty() && m_FreeList.front().first <= completedFenceValue)
        {
            DescriptorHandle result = m_FreeList.front().second;
            m_FreeList.pop();
            return result;
        }

        if (m_Pages.empty() || m_NextDescriptorIndex >= m_PageSize)
        {
            m_NextDescriptorIndex = 0;

            std::wstring name = L"DescriptorAllocatorPage" + std::to_wstring(m_Pages.size());
            m_Pages.push_back(std::make_unique<DescriptorHeap>(m_Device, m_DescriptorType, m_PageSize, false, name));
            DEBUG_LOG_INFO(L"Create %s; Size: %d; Type: %s", name.c_str(), m_PageSize, GetDescriptorHeapTypeName(m_DescriptorType));
        }

        return DescriptorHandle(m_Pages.back().get(), m_Pages.size() - 1, m_NextDescriptorIndex++);
    }

    void DescriptorAllocator::Free(const DescriptorHandle& handle, UINT64 fenceValue)
    {
        // TODO check if the handle is valid
        m_FreeList.emplace(fenceValue, handle);
    }

    DescriptorTable::DescriptorTable(DescriptorHeap* heap, UINT offset, UINT count)
        : m_Heap(heap), m_Offset(offset), m_Count(count)
    {

    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorTable::GetCpuHandle(UINT index) const
    {
        if (index < 0 || index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor table");
        }

        return m_Heap->GetCpuHandle(m_Offset + index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTable::GetGpuHandle(UINT index) const
    {
        if (index < 0 || index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor table");
        }

        return m_Heap->GetGpuHandle(m_Offset + index);
    }

    void DescriptorTable::Copy(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        if (destIndex < 0 || destIndex >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        m_Heap->Copy(m_Offset + destIndex, srcDescriptor);
    }

    DescriptorTableAllocator::SegmentData::SegmentData(UINT count, bool canRelease)
        : Count(count), FenceValue(0), CanRelease(canRelease), CreatedFrame(GetApp().GetFrameCount())
    {
    }

    DescriptorTableAllocator::SegmentData::SegmentData() : SegmentData(0) {}

    DescriptorTableAllocator::DescriptorTableAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT staticDescriptorCount, UINT dynamicDescriptorCapacity)
        : m_DynamicSegments{}, m_DynamicFront(0), m_DynamicRear(0), m_DynamicCapacity(dynamicDescriptorCapacity)
    {
        std::wstring name = GetDescriptorHeapTypeName(type) + std::wstring(L"_DescriptorTablePool");
        UINT capacity = dynamicDescriptorCapacity + staticDescriptorCount; // static 部分放在 dynamic 后面
        m_Heap = std::make_unique<DescriptorHeap>(device, type, capacity, true, name);
    }

    DescriptorTable DescriptorTableAllocator::AllocateDynamicTable(UINT descriptorCount, UINT64 completedFenceValue)
    {
        // 循环队列需要保留一个空间来区分队列满和队列空
        if (descriptorCount > m_DynamicCapacity - 1)
        {
            throw std::out_of_range("Dynamic descriptor table size exceeds the capacity of the allocator");
        }

        // 回收可以使用的空间
        decltype(m_DynamicSegments)::const_iterator it;
        while ((it = m_DynamicSegments.find(m_DynamicFront)) != m_DynamicSegments.cend())
        {
            if (!it->second.CanRelease || it->second.FenceValue > completedFenceValue)
            {
                break;
            }

            m_DynamicFront += it->second.Count;
            m_DynamicFront %= m_DynamicCapacity;
            m_DynamicSegments.erase(it);
        }

        bool canAllocate = false;

        if (m_DynamicFront <= m_DynamicRear)
        {
            UINT remaining = m_DynamicCapacity - m_DynamicRear;

            if (m_DynamicFront == 0)
            {
                // 空出一个位置来区分队列满和队列空
                if (remaining - 1 >= descriptorCount)
                {
                    canAllocate = true;
                }
            }
            else
            {
                if (remaining < descriptorCount)
                {
                    if (remaining > 0)
                    {
                        m_DynamicSegments.emplace(m_DynamicRear, SegmentData(remaining, true));
                    }
                    m_DynamicRear = 0; // 后面不够了，从头开始分配，之后 Front > Rear
                }
                else
                {
                    canAllocate = true;
                }
            }
        }

        if (!canAllocate && m_DynamicFront - m_DynamicRear - 1 >= descriptorCount)
        {
            canAllocate = true;
        }

        if (!canAllocate)
        {
            throw std::out_of_range("Descriptor table pool is full");
        }

        m_DynamicSegments.emplace(m_DynamicRear, SegmentData(descriptorCount));
        DescriptorTable table(m_Heap.get(), m_DynamicRear, descriptorCount);
        m_DynamicRear = (m_DynamicRear + descriptorCount) % m_DynamicCapacity;
        return table;
    }

    void DescriptorTableAllocator::ReleaseDynamicTable(const DescriptorTable& table, UINT64 fenceValue)
    {
        auto it = m_DynamicSegments.find(table.GetOffset());

        if (it != m_DynamicSegments.end())
        {
            assert(m_Heap->GetHeapPointer() == table.GetHeapPointer());
            assert(it->second.Count == table.GetCount());

            it->second.FenceValue = fenceValue;
            it->second.CanRelease = true;
        }
        else
        {
            DEBUG_LOG_ERROR("Attempt to release an invalid dynamic descriptor table");
        }
    }

    DescriptorTable DescriptorTableAllocator::GetStaticTable() const
    {
        return DescriptorTable(m_Heap.get(), m_DynamicCapacity, GetStaticDescriptorCount());
    }
}
