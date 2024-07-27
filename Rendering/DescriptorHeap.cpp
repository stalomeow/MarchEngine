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

    void DescriptorHeap::Append(D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
    {
        if (m_DynamicBufCapacity == 0)
        {
            throw std::out_of_range("The capacity of dynamic descriptor heap is zero");
        }

        if ((m_DynamicBufNext + 1) % m_DynamicBufCapacity == m_DynamicBufEnd)
        {
            throw std::out_of_range("Dynamic descriptor heap is full");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(m_DynamicBufNext, m_DescriptorSize);
        m_DynamicBufNext = (m_DynamicBufNext + 1) % m_DynamicBufCapacity;

        auto device = GetGfxManager().GetDevice();
        device->CopyDescriptorsSimple(1, handle, descriptor, m_Heap->GetDesc().Type);
    }

    void DescriptorHeap::Clear(UINT64 completedFenceValue, UINT64 nextFenceValue)
    {
        m_DynamicBufUsed.push(std::make_pair(nextFenceValue, m_DynamicBufNext));
        m_DynamicBufBase = m_DynamicBufNext; // 更新起点

        // 清理已经使用完成的空间
        while (!m_DynamicBufUsed.empty() && m_DynamicBufUsed.front().first <= completedFenceValue)
        {
            m_DynamicBufEnd = m_DynamicBufUsed.front().second;
            m_DynamicBufUsed.pop();

            DEBUG_LOG_INFO("Clear dynamic descriptor heap, completed-fence=%d", completedFenceValue);
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCpuHandleForDynamicHeapStart() const
    {
        if (GetDynamicCount() == 0)
        {
            throw std::out_of_range("Dynamic descriptor heap is empty");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_Heap->GetCPUDescriptorHandleForHeapStart());
        return handle.Offset(m_DynamicBufBase, m_DescriptorSize);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGpuHandleForDynamicHeapStart() const
    {
        if (GetDynamicCount() == 0)
        {
            throw std::out_of_range("Dynamic descriptor heap is empty");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle(m_Heap->GetGPUDescriptorHandleForHeapStart());
        return handle.Offset(m_DynamicBufBase, m_DescriptorSize);
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
}
