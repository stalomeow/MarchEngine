#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <unordered_map>

namespace dx12demo
{
    // Use ring buffer for dynamic descriptors
    class DescriptorHeap
    {
    public:
        DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType,
            UINT dynamicCapacity = 4096, UINT fixedCount = 0, bool shaderVisible = true);

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap.Get(); }

        UINT Append(D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
        void Clear();

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleForDynamicDescriptor(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleForDynamicDescriptor(UINT index) const;

        UINT GetDynamicCount() const { return m_DynamicBufNext - m_DynamicBufBase; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandleForFixedDescriptor(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandleForFixedDescriptor(UINT index) const;

        UINT GetFixedCount() const { return m_FixedCount; }

        UINT GetDescriptorSize() const { return m_DescriptorSize; }

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

    typedef std::pair<UINT, UINT> LinkedDescriptorSlot;

    // DynamicSize NonShaderVisible DescriptorHeap
    class LinkedDescriptorHeap
    {
    public:
        LinkedDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorCountPerPage = 4096);
        ~LinkedDescriptorHeap() = default;

        LinkedDescriptorSlot Allocate();
        void Free(const LinkedDescriptorSlot& slot);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const LinkedDescriptorSlot& slot) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const LinkedDescriptorSlot& slot) const;

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
        UINT m_DescriptorCountPerPage;

        std::vector<std::unique_ptr<DescriptorHeap>> m_HeapPages{};
        UINT m_NextDescriptorIndex = 0;
        std::queue<std::pair<UINT64, LinkedDescriptorSlot>> m_FreeList{};
    };

    typedef std::pair<D3D12_DESCRIPTOR_HEAP_TYPE, LinkedDescriptorSlot> DescriptorHandle;

    class DescriptorManager
    {
    public:
        static DescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        static void Free(const DescriptorHandle& handle);

        static D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const DescriptorHandle& handle);
        static D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const DescriptorHandle& handle);

    private:
        static void EnsureHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

        static std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<LinkedDescriptorHeap>> s_Heaps;
    };
}
