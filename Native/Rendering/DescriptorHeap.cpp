#include <directx/d3dx12.h>
#include <exception>
#include <stdexcept>
#include "Rendering/DescriptorHeap.h"
#include "Rendering/GfxManager.h"
#include "Rendering/DxException.h"
#include "Core/Debug.h"

namespace dx12demo
{
    DescriptorHeap::DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT dynamicCapacity, UINT fixedCount, bool shaderVisible)
        : m_DynamicBufBase(0)
        , m_DynamicBufNext(0)
        , m_DynamicBufEnd(0)
        , m_DynamicBufCapacity(dynamicCapacity)
        , m_DynamicBufUsed()
        , m_FixedCount(fixedCount)
    {
        auto device = GetGfxManager().GetDevice();
        m_DescriptorSize = device->GetDescriptorHandleIncrementSize(heapType);

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = dynamicCapacity + fixedCount; // 在最后追加 fixedCount 个固定的描述符
        desc.Type = heapType;
        desc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        THROW_IF_FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

#if defined(ENABLE_GFX_DEBUG_NAME)
        THROW_IF_FAILED(m_Heap->SetName(name.c_str()));
#else
        (name);
#endif
    }

    UINT DescriptorHeap::Append(D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
    {
        if (m_DynamicBufCapacity == 0)
        {
            throw std::out_of_range("The capacity of dynamic descriptor heap is zero");
        }

        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();

        // 清理已经使用完成的空间
        while (!m_DynamicBufUsed.empty() && m_DynamicBufUsed.front().first <= completedFenceValue)
        {
            m_DynamicBufEnd = m_DynamicBufUsed.front().second;
            m_DynamicBufUsed.pop();
        }

        if ((m_DynamicBufNext + 1) % m_DynamicBufCapacity == m_DynamicBufEnd)
        {
            throw std::out_of_range("Dynamic descriptor heap is full");
        }

        UINT index = m_DynamicBufNext;
        m_DynamicBufNext = (m_DynamicBufNext + 1) % m_DynamicBufCapacity;

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(index, m_DescriptorSize);

        auto device = GetGfxManager().GetDevice();
        device->CopyDescriptorsSimple(1, handle, descriptor, m_Heap->GetDesc().Type);
        return index - m_DynamicBufBase;
    }

    void DescriptorHeap::Clear()
    {
        UINT64 fenceValue = GetGfxManager().GetNextFenceValue();
        m_DynamicBufUsed.push(std::make_pair(fenceValue, m_DynamicBufNext));
        m_DynamicBufBase = m_DynamicBufNext; // 更新起点
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandleForDynamicDescriptor(UINT index) const
    {
        if (GetDynamicCount() == 0)
        {
            throw std::out_of_range("Dynamic descriptor heap is empty");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        return handle.Offset((m_DynamicBufBase + index) % m_DynamicBufCapacity, m_DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandleForDynamicDescriptor(UINT index) const
    {
        if (GetDynamicCount() == 0)
        {
            throw std::out_of_range("Dynamic descriptor heap is empty");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_Heap->GetGPUDescriptorHandleForHeapStart());
        return handle.Offset((m_DynamicBufBase + index) % m_DynamicBufCapacity, m_DescriptorSize);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandleForFixedDescriptor(UINT index) const
    {
        if (index < 0 || index >= m_FixedCount)
        {
            throw std::out_of_range("Index out of the range of fixed descriptor heap");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        return handle.Offset(m_DynamicBufCapacity + index, m_DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandleForFixedDescriptor(UINT index) const
    {
        if (index < 0 || index >= m_FixedCount)
        {
            throw std::out_of_range("Index out of the range of fixed descriptor heap");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_Heap->GetGPUDescriptorHandleForHeapStart());
        return handle.Offset(m_DynamicBufCapacity + index, m_DescriptorSize);
    }

    LinkedDescriptorHeap::LinkedDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorCountPerPage)
        : m_HeapType(heapType), m_DescriptorCountPerPage(descriptorCountPerPage)
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
                m_HeapType, 0, m_DescriptorCountPerPage, false));
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
        return m_HeapPages[slot.first]->GetCpuHandleForFixedDescriptor(slot.second);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE LinkedDescriptorHeap::GetGpuHandle(const LinkedDescriptorSlot& slot) const
    {
        return m_HeapPages[slot.first]->GetGpuHandleForFixedDescriptor(slot.second);
    }

    std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<LinkedDescriptorHeap>> DescriptorManager::s_Heaps{};

    DescriptorHandle DescriptorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    {
        EnsureHeap(heapType);
        return std::make_pair(heapType, s_Heaps[heapType]->Allocate());
    }

    void DescriptorManager::Free(const DescriptorHandle& handle)
    {
        EnsureHeap(handle.first);
        s_Heaps[handle.first]->Free(handle.second);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorManager::GetCpuHandle(const DescriptorHandle& handle)
    {
        EnsureHeap(handle.first);
        return s_Heaps[handle.first]->GetCpuHandle(handle.second);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorManager::GetGpuHandle(const DescriptorHandle& handle)
    {
        EnsureHeap(handle.first);
        return s_Heaps[handle.first]->GetGpuHandle(handle.second);
    }

    void DescriptorManager::EnsureHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    {
        if (s_Heaps.find(heapType) == s_Heaps.end())
        {
            s_Heaps.emplace(heapType, std::make_unique<LinkedDescriptorHeap>(heapType));
        }
    }
}
