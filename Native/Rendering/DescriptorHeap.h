#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdexcept>
#include <exception>
#include <unordered_set>

namespace dx12demo
{
    class DescriptorHeap
    {
    public:
        DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity, bool shaderVisible);
        virtual ~DescriptorHeap();

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;
        void Copy(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        UINT GetDescriptorSize() const { return m_DescriptorSize; }
        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Heap->GetDesc().Type; }
        UINT GetCapacity() const { return m_Heap->GetDesc().NumDescriptors; }

        bool IsShaderVisible() const
        {
            const D3D12_DESCRIPTOR_HEAP_FLAGS shaderVisibleFlag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
            return (desc.Flags & shaderVisibleFlag) == shaderVisibleFlag;
        }

    public:
        DescriptorHeap(const DescriptorHeap&) = delete;
        DescriptorHeap& operator=(const DescriptorHeap&) = delete;

    private:
        UINT m_DescriptorSize;
        ID3D12DescriptorHeap* m_Heap;
    };

    typedef std::pair<UINT, UINT> DescriptorHandle; // page-index, descriptor-index
    typedef std::pair<D3D12_DESCRIPTOR_HEAP_TYPE, DescriptorHandle> TypedDescriptorHandle;

    // shader-opaque descriptor allocator
    class DescriptorAllocator
    {
    public:
        DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType, UINT pageSize);
        ~DescriptorAllocator() = default;

        DescriptorHandle Allocate();
        void Free(const DescriptorHandle& handle);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const DescriptorHandle& handle) const;

        D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorType() const { return m_DescriptorType; }
        UINT GetPageSize() const { return m_PageSize; }

    public:
        DescriptorAllocator(const DescriptorAllocator&) = delete;
        DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
        UINT m_PageSize;

        UINT m_NextDescriptorIndex = 0;
        std::vector<std::unique_ptr<DescriptorHeap>> m_Pages{};
        std::queue<std::pair<UINT64, DescriptorHandle>> m_FreeList{};
    };

    // shader-opaque descriptor manager
    class DescriptorManager
    {
    public:
        TypedDescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
        void Free(const TypedDescriptorHandle& handle);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const TypedDescriptorHandle& handle);

    public:
        const UINT AllocatorPageSize = 1024;

    private:
        DescriptorAllocator* GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
        std::unique_ptr<DescriptorAllocator> m_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES]{};
    };

    class DescriptorTable
    {
        friend DescriptorTableAllocator;

    public:
        DescriptorTable(DescriptorHeap* heap, UINT offset, UINT count);
        ~DescriptorTable() = default;

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;
        void Copy(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

    private:
        DescriptorHeap* m_Heap;
        UINT m_Offset;
        UINT m_Count;
    };

    class DescriptorTableAllocator
    {
    public:
        DescriptorTableAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, UINT staticDescriptorCount, UINT dynamicDescriptorCapacity);
        ~DescriptorTableAllocator() = default;

        DescriptorTable AllocateDynamicTable(UINT descriptorCount);
        void ReleaseDynamicTables(UINT tableCount, const DescriptorTable* pTables, UINT64 fenceValue);

        DescriptorTable GetStaticTable() const;

        UINT GetStaticDescriptorCount() const { return m_Heap->GetCapacity() - m_DynamicCapacity; }
        UINT GetDynamicDescriptorCapacity() const { return m_DynamicCapacity; }
        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap->GetHeapPointer(); }

    public:
        DescriptorTableAllocator(const DescriptorTableAllocator&) = delete;
        DescriptorTableAllocator& operator=(const DescriptorTableAllocator&) = delete;

    private:
        struct ReleaseRange
        {
            UINT Offset;
            UINT Count;
            UINT64 FenceValue;

            bool operator>(const ReleaseRange& rhs) const
            {
                return FenceValue > rhs.FenceValue;
            }
        };

        std::unique_ptr<DescriptorHeap> m_Heap;
        std::unordered_set<UINT> m_DynamicUsedIndices;
        std::priority_queue<ReleaseRange, std::vector<ReleaseRange>, std::greater<ReleaseRange>> m_DynamicReleaseQueue;
        UINT m_DynamicSearchStart;
        UINT m_DynamicCapacity;
    };
}
