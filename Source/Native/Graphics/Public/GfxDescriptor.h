#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <bitset>

#undef min
#undef max

namespace march
{
    class GfxDevice;

    struct GfxDescriptorHeapDesc
    {
        D3D12_DESCRIPTOR_HEAP_TYPE Type;
        uint32_t Capacity;
        bool ShaderVisible;
    };

    class GfxDescriptorHeap
    {
    public:
        GfxDescriptorHeap(GfxDevice* device, const std::string& name, const GfxDescriptorHeapDesc& desc);
        ~GfxDescriptorHeap();

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;
        void CopyFrom(const D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptors, uint32_t numDescriptors, uint32_t destStartIndex) const;

        GfxDevice* GetDevice() const { return m_Device; }
        ID3D12DescriptorHeap* GetD3DDescriptorHeap() const { return m_Heap.Get(); }
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
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
        uint32_t m_IncrementSize;
    };

    class GfxOfflineDescriptorAllocator
    {
        friend class GfxOfflineDescriptor;

    public:
        GfxOfflineDescriptorAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t pageSize);

        GfxOfflineDescriptor Allocate();
        void Release(const GfxOfflineDescriptor& descriptor);

        void CleanUpAllocations();

        const GfxOfflineDescriptor& GetDefault() const;

        GfxDevice* GetDevice() const { return m_Device; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }

    private:
        GfxDevice* m_Device;
        D3D12_DESCRIPTOR_HEAP_TYPE m_Type;

        uint32_t m_NextDescriptorIndex;
        std::vector<std::unique_ptr<GfxDescriptorHeap>> m_Pages;
        std::queue<std::pair<uint64_t, GfxOfflineDescriptor>> m_ReleaseQueue;
    };

    class GfxOfflineDescriptor
    {
    public:
        GfxOfflineDescriptor() = default;
        GfxOfflineDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle) : m_Handle(handle), m_Version(0) {}

        uint32_t GetVersion() const { return m_Version; }
        D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const { return m_Handle; }

        // D3D12_CPU_DESCRIPTOR_HANDLE 本质上是一个指针，用 version 表示指针指向的内容的版本
        // 当内容变化时，增加 version，使以前的缓存失效
        void IncrementVersion() { m_Version++; }

        bool operator==(const GfxOfflineDescriptor& other) const
        {
            return m_Handle.ptr == other.m_Handle.ptr && m_Version == other.m_Version;
        }

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_Handle;
        uint32_t m_Version;
    };

    template <size_t Capacity>
    class GfxOfflineDescriptorCache
    {
    public:
        GfxOfflineDescriptorCache() = default;

        // 当 root signature 变化时，全部清空
        // 如果 root signature 没变，可以通过设置某几个 dirty 位来标记 descriptor 是否需要更新，没变则可以不重新设置 root descriptor table
        void Clear()
        {
            m_Count = 0;
            m_Dirty.reset();
            memset(&m_Descriptors, 0, sizeof(m_Descriptors));
        }

        void Apply()
        {
            m_Dirty.reset();
        }

        bool IsDirty() const
        {
            return m_Dirty.any();
        }

        size_t GetCount() const
        {
            return m_Count;
        }

        constexpr size_t GetCapacity() const
        {
            return Capacity;
        }

        const GfxOfflineDescriptor& Get(size_t index) const
        {
            return m_Descriptors[index];
        }

        void Set(size_t index, const GfxOfflineDescriptor& descriptor)
        {
            if (index < m_Count && m_Descriptors[index] == descriptor)
            {
                return;
            }

            m_Count = std::max(m_Count, index + 1);
            m_Dirty.set(index);
            m_Descriptors[index] = descriptor;
        }

    private:
        size_t m_Count; // 设置的最大 index + 1
        std::bitset<Capacity> m_Dirty;
        GfxOfflineDescriptor m_Descriptors[Capacity];
    };

    enum class GfxDescriptorTableType
    {
        CbvSrvUav = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        Sampler = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
    };

    class GfxOnlineDescriptorHeapAllocator
    {

    };

    class GfxDescriptorTable
    {
    public:
        GfxDescriptorTable() = default;
        GfxDescriptorTable(GfxDescriptorHeap* heap, uint32_t offset, uint32_t count);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;
        void Copy(uint32_t destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const;

        GfxDescriptorTableType GetType() const { return static_cast<GfxDescriptorTableType>(m_Heap->GetType()); }
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
        GfxDescriptorTableAllocator(GfxDevice* device, GfxDescriptorTableType type, uint32_t staticDescriptorCount, uint32_t dynamicDescriptorCapacity);
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

        uint32_t GetDynamicFront() const { return m_DynamicFront; }
        uint32_t GetDynamicRear() const { return m_DynamicRear; }

    private:
        std::unique_ptr<GfxDescriptorHeap> m_Heap;
        std::queue<std::pair<uint64_t, uint32_t>> m_ReleaseQueue;
        uint32_t m_DynamicFront;
        uint32_t m_DynamicRear;
        uint32_t m_DynamicCapacity;
    };
}
