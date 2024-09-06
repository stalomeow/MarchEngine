#include <directx/d3dx12.h>
#include "Rendering/DescriptorHeap.h"
#include "Rendering/GfxManager.h"
#include "Rendering/DxException.h"
#include "Core/Debug.h"

namespace dx12demo
{
    DescriptorHeap::DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT capacity, bool shaderVisible)
    {
        auto device = GetGfxManager().GetDevice();
        m_DescriptorSize = device->GetDescriptorHandleIncrementSize(heapType);

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = capacity;
        desc.Type = heapType;
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

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuDescriptorHandle(UINT index) const
    {
        if (index < 0 || index >= GetCapacity())
        {
            throw std::out_of_range("Index out of the range of descriptor heap");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        return handle.Offset(index, m_DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuDescriptorHandle(UINT index) const
    {
        if (index < 0 || index >= GetCapacity())
        {
            throw std::out_of_range("Index out of the range of descriptor heap");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_Heap->GetGPUDescriptorHandleForHeapStart());
        return handle.Offset(index, m_DescriptorSize);
    }

    void DescriptorHeap::CopyDescriptor(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        auto device = GetGfxManager().GetDevice();
        D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor = GetCpuDescriptorHandle(destIndex);
        device->CopyDescriptorsSimple(1, destDescriptor, srcDescriptor, GetType());
    }

    LinkedDescriptorHeap::LinkedDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorCountPerPage, bool shaderVisible)
        : m_HeapType(heapType), m_DescriptorCountPerPage(descriptorCountPerPage), m_IsShaderVisible(shaderVisible)
    {

    }

    LinkedDescriptorSlot LinkedDescriptorHeap::Allocate()
    {
        if (!m_FreeList.empty() && m_FreeList.front().first <= GetGfxManager().GetCompletedFenceValue())
        {
            LinkedDescriptorSlot result = m_FreeList.front().second;
            m_FreeList.pop();
            return result;
        }

        if (m_HeapPages.empty() || m_NextDescriptorIndex >= m_DescriptorCountPerPage)
        {
            m_HeapPages.push_back(std::make_unique<DescriptorHeap>(L"LinkedDescriptorHeapPage" + std::to_wstring(m_HeapPages.size()),
                m_HeapType, m_DescriptorCountPerPage, m_IsShaderVisible));
            m_NextDescriptorIndex = 0;
        }

        return std::make_pair(m_HeapPages.size() - 1, m_NextDescriptorIndex++);
    }

    void LinkedDescriptorHeap::Free(const LinkedDescriptorSlot& slot)
    {
        m_FreeList.emplace(GetGfxManager().GetNextFenceValue(), slot);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE LinkedDescriptorHeap::GetCpuHandle(const LinkedDescriptorSlot& slot) const
    {
        return m_HeapPages[slot.first]->GetCpuDescriptorHandle(slot.second);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE LinkedDescriptorHeap::GetGpuHandle(const LinkedDescriptorSlot& slot) const
    {
        return m_HeapPages[slot.first]->GetGpuDescriptorHandle(slot.second);
    }

    std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<LinkedDescriptorHeap>> DescriptorManager::s_Heaps{};

    ShaderOpaqueDescriptorHandle DescriptorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    {
        return std::make_pair(heapType, GetHeap(heapType)->Allocate());
    }

    void DescriptorManager::Free(const ShaderOpaqueDescriptorHandle& handle)
    {
        GetHeap(handle.first)->Free(handle.second);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetCpuHandle(const ShaderOpaqueDescriptorHandle& handle)
    {
        return GetHeap(handle.first)->GetCpuHandle(handle.second);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetGpuHandle(const ShaderOpaqueDescriptorHandle& handle)
    {
        return GetHeap(handle.first)->GetGpuHandle(handle.second);
    }

    LinkedDescriptorHeap* DescriptorManager::GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    {
        auto it = s_Heaps.find(heapType);
        LinkedDescriptorHeap* result;

        if (it == s_Heaps.end())
        {
            auto heap = std::make_unique<LinkedDescriptorHeap>(heapType, 4096, false);
            result = heap.get();
            s_Heaps.emplace(heapType, std::move(heap));
        }
        else
        {
            result = it->second.get();
        }

        return result;
    }

    DescriptorHeapSpan::DescriptorHeapSpan(DescriptorHeap* heap, UINT offset, UINT count)
        : m_Heap(heap), m_Offset(offset), m_Count(count)
    {

    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapSpan::GetCpuDescriptorHandle(UINT index) const
    {
        if (index < 0 || index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        return m_Heap->GetCpuDescriptorHandle(m_Offset + index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapSpan::GetGpuDescriptorHandle(UINT index) const
    {
        if (index < 0 || index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        return m_Heap->GetGpuDescriptorHandle(m_Offset + index);
    }

    void DescriptorHeapSpan::CopyDescriptor(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        if (destIndex < 0 || destIndex >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        m_Heap->CopyDescriptor(m_Offset + destIndex, srcDescriptor);
    }

    DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT pageSize)
        : m_HeapType(heapType), m_PageSize(pageSize)
    {

    }

    DescriptorHeapSpan DynamicDescriptorHeap::Allocate(UINT count)
    {
        if (count > m_PageSize)
        {
            throw std::out_of_range("Requested descriptor count exceeds the page size of dynamic descriptor heap");
        }

        if (m_ActiveHeaps.empty() || m_ActiveHeaps.back()->GetCapacity() - m_Offset < count)
        {
            m_ActiveHeaps.push_back(RequestNewHeap());
            m_Offset = 0;
        }

        DescriptorHeapSpan span(m_ActiveHeaps.back(), m_Offset, count);
        m_Offset += count;
        return span;
    }

    void DynamicDescriptorHeap::Flush(UINT64 fenceValue)
    {
        for (DescriptorHeap* heap : m_ActiveHeaps)
        {
            m_PendingHeaps.emplace(fenceValue, heap);
        }

        m_ActiveHeaps.clear();
        m_Offset = 0;
    }

    DescriptorHeap* DynamicDescriptorHeap::RequestNewHeap()
    {
        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();

        if (!m_PendingHeaps.empty() && m_PendingHeaps.front().first <= completedFenceValue)
        {
            DescriptorHeap* heap = m_PendingHeaps.front().second;
            m_PendingHeaps.pop();
            return heap;
        }

        m_AllHeaps.push_back(std::make_unique<DescriptorHeap>(L"DynamicDescriptorHeapPage", m_HeapType, m_PageSize, true));
        return m_AllHeaps.back().get();
    }

    DescriptorList::DescriptorList(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT capacity, bool shaderVisible)
        : m_HeapType(heapType), m_Capacity(capacity), m_IsShaderVisible(shaderVisible)
    {

    }

    UINT DescriptorList::Append(D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
    {
        if (m_CurrentHeap == nullptr)
        {
            m_AllHeaps.push_back(std::make_unique<DescriptorHeap>(L"DescriptorListHeap", m_HeapType, m_Capacity, m_IsShaderVisible));
            m_CurrentHeap = m_AllHeaps.back().get();
            m_NextIndex = 0;
        }

        if (m_NextIndex >= m_Capacity)
        {
            throw std::out_of_range("Descriptor list is full");
        }

        UINT index = m_NextIndex++;
        m_CurrentHeap->CopyDescriptor(index, descriptor);
        return index;
    }

    void DescriptorList::Flush(UINT64 fenceValue)
    {
        m_PendingHeaps.emplace(fenceValue, m_CurrentHeap);
        m_CurrentHeap = nullptr;
        m_NextIndex = 0;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorList::GetCpuHandle(UINT index) const
    {
        return {};
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorList::GetGpuHandle(UINT index) const
    {
        return {};
    }
}
