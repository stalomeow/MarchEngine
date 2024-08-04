#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <queue>

namespace dx12demo
{
    // Use ring buffer for dynamic descriptors
    class DescriptorHeap
    {
    public:
        DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
            UINT dynamicCapacity = 4096, UINT fixedCount = 0, bool shaderVisible = true);

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap.Get(); }

        void Append(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, UINT64 completedFenceValue);
        void Clear(UINT64 fenceValue);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleForDynamicHeapStart() const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleForDynamicHeapStart() const;

        UINT GetDynamicCount() const { return m_DynamicBufNext - m_DynamicBufBase; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleForFixedDescriptor(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleForFixedDescriptor(UINT index) const;

        UINT GetFixedCount() const { return m_FixedCount; }

    private:
        UINT m_DynamicBufBase; // 本次的起点
        UINT m_DynamicBufNext; // 相当于循环队列里的 rear
        UINT m_DynamicBufEnd;  // 相当于循环队列里的 front
        UINT m_DynamicBufCapacity;
        std::queue<std::pair<UINT64, UINT>> m_DynamicBufUsed; // <fence-value, buf-end>

        UINT m_FixedCount;

        UINT m_DescriptorSize;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
    };
}
