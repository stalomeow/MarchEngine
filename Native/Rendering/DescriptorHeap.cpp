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
            throw std::out_of_range("Index out of the range of descriptor heap");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        return handle.Offset(index, m_DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandle(UINT index) const
    {
        if (index < 0 || index >= GetCapacity())
        {
            throw std::out_of_range("Index out of the range of descriptor heap");
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

    DescriptorTableAllocator::DescriptorTableAllocator(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT staticDescriptorCount, UINT dynamicDescriptorCapacity)
        : m_DynamicUsedIndices{}, m_DynamicReleaseQueue{}, m_DynamicSearchStart(0), m_DynamicCapacity(dynamicDescriptorCapacity)
    {
        std::wstring name = GetDescriptorHeapTypeName(type) + std::wstring(L"_DescriptorTablePool");
        UINT capacity = dynamicDescriptorCapacity + staticDescriptorCount; // static 部分放在 dynamic 后面
        m_Heap = std::make_unique<DescriptorHeap>(device, type, capacity, true, name);
    }

    DescriptorTable DescriptorTableAllocator::AllocateDynamicTable(UINT descriptorCount, UINT64 completedFenceValue)
    {
        bool released = false;
        UINT minReleasedOffset = 0;

        while (!m_DynamicReleaseQueue.empty() && m_DynamicReleaseQueue.top().FenceValue <= completedFenceValue)
        {
            const ReleaseRange& range = m_DynamicReleaseQueue.top();

            if (!released || range.Offset < minReleasedOffset)
            {
                minReleasedOffset = range.Offset;
            }

            for (UINT i = 0; i < range.Count; i++)
            {
                m_DynamicUsedIndices.erase(range.Offset + i);
            }

            released = true;
            m_DynamicReleaseQueue.pop();
        }

        if (released)
        {
            m_DynamicSearchStart = minReleasedOffset;
        }

        UINT freeCount = 0;

        for (UINT i = m_DynamicSearchStart; i < m_DynamicCapacity; i++)
        {
            if (m_DynamicUsedIndices.count(i) > 0)
            {
                freeCount = 0;
            }
            else
            {
                freeCount++;
            }

            if (freeCount >= descriptorCount)
            {
                UINT offset = i - descriptorCount + 1;

                for (UINT j = offset; j <= i; j++)
                {
                    m_DynamicUsedIndices.insert(j);
                }

                m_DynamicSearchStart = offset + descriptorCount;
                return DescriptorTable(m_Heap.get(), offset, descriptorCount);
            }
        }

        throw std::runtime_error("Failed to allocate dynamic descriptor table");
    }

    void DescriptorTableAllocator::ReleaseDynamicTables(UINT tableCount, const DescriptorTable* pTables, UINT64 fenceValue)
    {
        for (int i = 0; i < tableCount; i++)
        {
            m_DynamicReleaseQueue.push({ pTables[i].GetOffset(), pTables[i].GetCount(), fenceValue});
        }
    }

    DescriptorTable DescriptorTableAllocator::GetStaticTable() const
    {
        return DescriptorTable(m_Heap.get(), m_DynamicCapacity, GetStaticDescriptorCount());
    }
}
