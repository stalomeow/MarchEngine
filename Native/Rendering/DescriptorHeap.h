#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <exception>

namespace dx12demo
{
    class DescriptorHeap
    {
    public:
        DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT capacity, bool shaderVisible);
        virtual ~DescriptorHeap();

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT index) const;
        void CopyDescriptor(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap; }

        UINT GetDescriptorSize() const { return m_DescriptorSize; }

        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Heap->GetDesc().Type; }

        UINT GetCapacity() const { return m_Heap->GetDesc().NumDescriptors; }

        bool IsShaderVisible() const
        {
            const D3D12_DESCRIPTOR_HEAP_FLAGS shaderVisibleFlag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
            return (desc.Flags & shaderVisibleFlag) == shaderVisibleFlag;
        }

    private:
        UINT m_DescriptorSize;
        ID3D12DescriptorHeap* m_Heap;
    };

    typedef std::pair<UINT, UINT> LinkedDescriptorSlot; // page-index, descriptor-index

    class LinkedDescriptorHeap
    {
    public:
        LinkedDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT descriptorCountPerPage, bool shaderVisible);
        ~LinkedDescriptorHeap() = default;

        LinkedDescriptorSlot Allocate();
        void Free(const LinkedDescriptorSlot& slot);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const LinkedDescriptorSlot& slot) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const LinkedDescriptorSlot& slot) const;

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
        UINT m_DescriptorCountPerPage;
        bool m_IsShaderVisible;

        std::vector<std::unique_ptr<DescriptorHeap>> m_HeapPages{};
        UINT m_NextDescriptorIndex = 0;
        std::queue<std::pair<UINT64, LinkedDescriptorSlot>> m_FreeList{};
    };

    typedef std::pair<D3D12_DESCRIPTOR_HEAP_TYPE, LinkedDescriptorSlot> ShaderOpaqueDescriptorHandle;

    class DescriptorManager
    {
    public:
        static ShaderOpaqueDescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        static void Free(const ShaderOpaqueDescriptorHandle& handle);

        static D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const ShaderOpaqueDescriptorHandle& handle);
        static D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const ShaderOpaqueDescriptorHandle& handle);

    private:
        static LinkedDescriptorHeap* GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);

        static std::unordered_map<D3D12_DESCRIPTOR_HEAP_TYPE, std::unique_ptr<LinkedDescriptorHeap>> s_Heaps;
    };

    class DescriptorHeapSpan
    {
    public:
        DescriptorHeapSpan(DescriptorHeap* heap, UINT offset, UINT count);
        ~DescriptorHeapSpan() = default;

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuDescriptorHandle(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(UINT index) const;
        void CopyDescriptor(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap->GetHeapPointer(); }

    private:
        DescriptorHeap* m_Heap;
        UINT m_Offset;
        UINT m_Count;
    };

    class DynamicDescriptorHeap
    {
    public:
        DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT pageSize);
        ~DynamicDescriptorHeap() = default;

        DescriptorHeapSpan Allocate(UINT count);
        void Flush(UINT64 fenceValue);

    private:
        DescriptorHeap* RequestNewHeap();

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
        UINT m_PageSize;

        std::vector<std::unique_ptr<DescriptorHeap>> m_AllHeaps{};
        std::queue<std::pair<UINT64, DescriptorHeap*>> m_PendingHeaps{};
        std::vector<DescriptorHeap*> m_ActiveHeaps{};
        UINT m_Offset = 0;
    };

    class DescriptorList
    {
    public:
        DescriptorList(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT capacity, bool shaderVisible);
        ~DescriptorList() = default;

        UINT Append(D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
        void Flush(UINT64 fenceValue);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_CurrentHeap->GetHeapPointer(); }

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
        UINT m_Capacity;
        bool m_IsShaderVisible;

        std::vector<std::unique_ptr<DescriptorHeap>> m_AllHeaps{};
        std::queue<std::pair<UINT64, DescriptorHeap*>> m_PendingHeaps{};
        DescriptorHeap* m_CurrentHeap = nullptr;
        UINT m_NextIndex = 0;
    };
}
