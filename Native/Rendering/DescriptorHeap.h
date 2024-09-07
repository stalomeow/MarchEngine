#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdexcept>
#include <exception>

#define THROW_INVALID_DESCRIPTOR_HEAP_REGION throw std::invalid_argument("Invalid descriptor heap region")

namespace dx12demo
{
    enum class DescriptorHeapRegion
    {
        Fixed,   // 固定长度的区域，类似数组
        Dynamic, // 动态长度的区域，类似循环队列
    };

    class DescriptorHeap
    {
    public:
        DescriptorHeap(const std::wstring& name, D3D12_DESCRIPTOR_HEAP_TYPE type,
            UINT fixedCapacity, UINT dynamicCapacity, bool shaderVisible);
        virtual ~DescriptorHeap();

        UINT Append();
        void Flush();

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(DescriptorHeapRegion region, UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(DescriptorHeapRegion region, UINT index) const;
        void Copy(DescriptorHeapRegion region, UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        bool IsFull(DescriptorHeapRegion region) const
        {
            switch (region)
            {
            case dx12demo::DescriptorHeapRegion::Fixed:
                return true;
            case dx12demo::DescriptorHeapRegion::Dynamic:
                return ((m_DynamicRear + 1) % m_DynamicCapacity) == m_DynamicFront;
            default:
                THROW_INVALID_DESCRIPTOR_HEAP_REGION;
            }
        }

        UINT GetCount(DescriptorHeapRegion region) const
        {
            switch (region)
            {
            case dx12demo::DescriptorHeapRegion::Fixed:
                return m_FixedCapacity;
            case dx12demo::DescriptorHeapRegion::Dynamic:
                return (m_DynamicRear - m_DynamicBase + m_DynamicCapacity) % m_DynamicCapacity;
            default:
                THROW_INVALID_DESCRIPTOR_HEAP_REGION;
            }
        }

        UINT GetCapacity(DescriptorHeapRegion region) const
        {
            switch (region)
            {
            case dx12demo::DescriptorHeapRegion::Fixed:
                return m_FixedCapacity;
            case dx12demo::DescriptorHeapRegion::Dynamic:
                return m_DynamicCapacity - 1; // 循环队列需要预留一个空位
            default:
                THROW_INVALID_DESCRIPTOR_HEAP_REGION;
            }
        }

        UINT GetDescriptorSize() const { return m_DescriptorSize; }
        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const { return m_Heap->GetDesc().Type; }

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
        UINT m_DynamicBase; // 本次的起点
        UINT m_DynamicRear;
        UINT m_DynamicFront;
        UINT m_DynamicCapacity;
        std::queue<std::pair<UINT64, UINT>> m_DynamicUsed; // fence-value, front

        UINT m_FixedCapacity;

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
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const DescriptorHandle& handle) const;

        D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorType() const { return m_DescriptorType; }
        UINT GetPageSize() const { return m_PageSize; }

    public:
        DescriptorAllocator(const DescriptorAllocator&) = delete;
        DescriptorAllocator& operator=(const DescriptorAllocator&) = delete;

    private:
        D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
        UINT m_PageSize;

        std::vector<std::unique_ptr<DescriptorHeap>> m_Pages{};
        std::queue<std::pair<UINT64, DescriptorHandle>> m_FreeList{};
    };

    // shader-opaque descriptor manager
    class DescriptorManager
    {
    public:
        static TypedDescriptorHandle Allocate(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
        static void Free(const TypedDescriptorHandle& handle);

        static D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(const TypedDescriptorHandle& handle);
        static D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(const TypedDescriptorHandle& handle);

    public:
        static const UINT AllocatorPageSize = 1024;

    private:
        static DescriptorAllocator* GetAllocator(D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
        static std::unique_ptr<DescriptorAllocator> s_Allocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    };

    // shader-visible descriptor heap span
    class DescriptorHeapSpan
    {
    public:
        DescriptorHeapSpan(DescriptorHeap* heap, UINT offset, UINT count);
        ~DescriptorHeapSpan() = default;

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(UINT index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT index) const;
        void Copy(UINT destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        ID3D12DescriptorHeap* GetHeapPointer() const { return m_Heap->GetHeapPointer(); }

    private:
        DescriptorHeap* m_Heap;
        UINT m_Offset;
        UINT m_Count;
    };

    // shader-visible descriptor heap allocator
    class DescriptorHeapAllocator
    {
    public:
        DescriptorHeapAllocator(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT pageSize);
        ~DescriptorHeapAllocator() = default;

        DescriptorHeapSpan Allocate(UINT count);
        void Flush(UINT64 fenceValue);

    public:
        DescriptorHeapAllocator(const DescriptorHeapAllocator&) = delete;
        DescriptorHeapAllocator& operator=(const DescriptorHeapAllocator&) = delete;

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
}
