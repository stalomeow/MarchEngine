#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

namespace march
{
    class GfxDevice;

    class GfxDescriptorHeap
    {
    public:
        GfxDescriptorHeap(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity, bool shaderVisible, const std::string& name);
        ~GfxDescriptorHeap();

        GfxDescriptorHeap(const GfxDescriptorHeap&) = delete;
        GfxDescriptorHeap& operator=(const GfxDescriptorHeap&) = delete;

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;
        void Copy(uint32_t destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        uint32_t GetIncrementSize() const { return m_IncrementSize; }
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return m_Heap; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Heap->GetDesc().Type; }
        uint32_t GetCapacity() const { return static_cast<uint32_t>(m_Heap->GetDesc().NumDescriptors); }

        bool IsShaderVisible() const
        {
            const D3D12_DESCRIPTOR_HEAP_FLAGS shaderVisibleFlag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
            return (desc.Flags & shaderVisibleFlag) == shaderVisibleFlag;
        }

    private:
        GfxDevice* m_Device;
        uint32_t m_IncrementSize;
        ID3D12DescriptorHeap* m_Heap;
    };

    class GfxDescriptorHandle
    {
        friend GfxDescriptorAllocator;

    public:
        GfxDescriptorHandle() = default;
        GfxDescriptorHandle(GfxDescriptorHeap* heap, uint32_t pageIndex, uint32_t heapIndex);

        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Heap->GetType(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const { return m_Heap->GetCpuHandle(m_HeapIndex); }

    private:
        GfxDescriptorHeap* m_Heap;
        uint32_t m_PageIndex;
        uint32_t m_HeapIndex;
    };

    // shader-opaque descriptor allocator
    class GfxDescriptorAllocator
    {
    public:
        GfxDescriptorAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorType);
        ~GfxDescriptorAllocator() = default;

        GfxDescriptorAllocator(const GfxDescriptorAllocator&) = delete;
        GfxDescriptorAllocator& operator=(const GfxDescriptorAllocator&) = delete;

        void BeginFrame();
        void EndFrame(uint64_t fenceValue);
        GfxDescriptorHandle Allocate();
        void Free(const GfxDescriptorHandle& handle);

        D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorType() const { return m_DescriptorType; }

    public:
        static const uint32_t PageSize = 1024;

    private:
        GfxDevice* m_Device;
        D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;

        uint32_t m_NextDescriptorIndex;
        std::vector<std::unique_ptr<GfxDescriptorHeap>> m_Pages;
        std::vector<GfxDescriptorHandle> m_FrameFreeList; // 当前帧释放的描述符，之后会添加到 m_ReleaseQueue
        std::queue<std::pair<uint64_t, GfxDescriptorHandle>> m_ReleaseQueue;
    };

    class GfxDescriptorTable
    {
    public:
        GfxDescriptorTable() = default;
        GfxDescriptorTable(GfxDescriptorHeap* heap, uint32_t offset, uint32_t count);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;
        void Copy(uint32_t destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Heap->GetType(); }
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return m_Heap->GetD3D12DescriptorHeap(); }
        uint32_t GetOffset() const { return m_Offset; }
        uint32_t GetCount() const { return m_Count; }

    private:
        GfxDescriptorHeap* m_Heap;
        uint32_t m_Offset;
        uint32_t m_Count;
    };

    class GfxDescriptorTableAllocator
    {
    public:
        GfxDescriptorTableAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t staticDescriptorCount, uint32_t dynamicDescriptorCapacity);
        ~GfxDescriptorTableAllocator() = default;

        GfxDescriptorTableAllocator(const GfxDescriptorTableAllocator&) = delete;
        GfxDescriptorTableAllocator& operator=(const GfxDescriptorTableAllocator&) = delete;

        void BeginFrame();
        void EndFrame(uint64_t fenceValue);
        GfxDescriptorTable AllocateDynamicTable(uint32_t descriptorCount);

        GfxDescriptorTable GetStaticTable() const;

        uint32_t GetStaticDescriptorCount() const { return m_Heap->GetCapacity() - m_DynamicCapacity; }
        uint32_t GetDynamicDescriptorCapacity() const { return m_DynamicCapacity; }
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const { return m_Heap->GetD3D12DescriptorHeap(); }

        struct SegmentData
        {
            uint32_t Count;
            uint64_t FenceValue;
            bool CanRelease;
            uint64_t CreatedFrame;

            SegmentData(uint32_t count, bool canRelease = false);
            SegmentData();
        };

        const std::map<uint32_t, SegmentData>& GetDynamicSegments() const { return m_DynamicSegments; }

    private:
        std::unique_ptr<GfxDescriptorHeap> m_Heap;
        // std::vector<GfxDescriptorTable> m_UsedDynamicTables;
        std::map<uint32_t, SegmentData> m_DynamicSegments; // key is the offset of the segment
        uint32_t m_DynamicFront;
        uint32_t m_DynamicRear;
        uint32_t m_DynamicCapacity;
    };
}
