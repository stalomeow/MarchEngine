#pragma once

#include "Engine/Object.h"
#include "Engine/Allocator.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <queue>

namespace march
{
    class GfxDevice;
    class GfxResourceAllocator;

    // 记录不同分配器分配的信息
    union GfxResourceAllocation
    {
        BuddyAllocation Buddy;
    };

    class GfxResource final : public ThreadSafeRefCountedObject
    {
    public:
        GfxResource(GfxDevice* device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state);
        GfxResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES state, GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation);
        ~GfxResource();

        GfxDevice* GetDevice() const { return m_Device; }
        GfxResourceAllocator* GetAllocator() const { return m_Allocator; }
        ID3D12Resource* GetD3DResource() const { return m_Resource.Get(); }
        D3D12_RESOURCE_DESC GetD3DResourceDesc() const { return m_Resource->GetDesc(); }

        D3D12_RESOURCE_STATES GetState() const { return m_State; }
        void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

        GfxResource(const GfxResource&) = delete;
        GfxResource& operator=(const GfxResource&) = delete;

        GfxResource(GfxResource&&) = delete;
        GfxResource& operator=(GfxResource&&) = delete;

    private:
        GfxDevice* m_Device;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
        D3D12_RESOURCE_STATES m_State;

        // 分配整个资源使用的 allocator，可以没有
        GfxResourceAllocator* m_Allocator;
        GfxResourceAllocation m_Allocation;
    };

    class GfxResourceSpan final
    {
    public:
        GfxResourceSpan(RefCountPtr<GfxResource> resource, uint32_t bufferOffset, uint32_t bufferSize);
        GfxResourceSpan(RefCountPtr<GfxResource> resource) : GfxResourceSpan(resource, 0, 0) {}
        GfxResourceSpan() : GfxResourceSpan(nullptr, 0, 0) {}
        ~GfxResourceSpan() { Release(); }

        GfxResourceSpan MakeBufferSlice(uint32_t offset, uint32_t size, GfxResourceAllocator* allocator, const GfxResourceAllocation& allocation) const;

        RefCountPtr<GfxResource> GetResource() const { return m_Resource; }
        ID3D12Resource* GetD3DResource() const { return m_Resource->GetD3DResource(); }
        D3D12_RESOURCE_DESC GetD3DResourceDesc() const { return m_Resource->GetD3DResourceDesc(); }
        GfxDevice* GetDevice() const { return m_Resource->GetDevice(); }
        GfxResourceAllocator* GetSubAllocator() const { return m_Allocator; }

        uint32_t GetBufferOffset() const { return m_BufferOffset; }
        uint32_t GetBufferSize() const { return m_BufferSize; }

        operator bool() const { return m_Resource != nullptr; }

        GfxResourceSpan(const GfxResourceSpan&) = delete;
        GfxResourceSpan& operator=(const GfxResourceSpan&) = delete;

        GfxResourceSpan(GfxResourceSpan&&) noexcept;
        GfxResourceSpan& operator=(GfxResourceSpan&&);

    private:
        RefCountPtr<GfxResource> m_Resource;

        // 分配部分资源 (suballocation) 使用的 allocator，可以没有
        GfxResourceAllocator* m_Allocator;
        GfxResourceAllocation m_Allocation;

        // Buffer 可以使用 suballocation
        // 如果不是 Buffer，忽略这两个字段
        uint32_t m_BufferOffset;
        uint32_t m_BufferSize;

        void Release();
    };

    class GfxResourceAllocator
    {
    public:
        virtual ~GfxResourceAllocator() = default;

        virtual void DeferredRelease(const GfxResourceAllocation& allocation) = 0;
        virtual void CleanUpAllocations() = 0;

        virtual D3D12_HEAP_PROPERTIES GetHeapProperties() const = 0;
        virtual D3D12_HEAP_FLAGS GetHeapFlags() const = 0;

        GfxDevice* GetDevice() const { return m_Device; }
        bool IsHeapCpuAccessible() const { return CD3DX12_HEAP_PROPERTIES(GetHeapProperties()).IsCPUAccessible(); }

    protected:
        GfxResourceAllocator(GfxDevice* device) : m_Device(device) {}

    private:
        GfxDevice* m_Device;
    };

    // 用于分配完整的资源
    class GfxCompleteResourceAllocator : public GfxResourceAllocator
    {
    public:
        virtual GfxResourceSpan Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) = 0;

        void DeferredRelease(const GfxResourceAllocation& allocation) override;
        void CleanUpAllocations() override;

        D3D12_HEAP_PROPERTIES GetHeapProperties() const override { return CD3DX12_HEAP_PROPERTIES(m_HeapType); }
        D3D12_HEAP_FLAGS GetHeapFlags() const override { return m_HeapFlags; }

    protected:
        GfxCompleteResourceAllocator(GfxDevice* device, D3D12_HEAP_TYPE heapType, D3D12_HEAP_FLAGS heapFlags);

        GfxResourceSpan CreateResourceSpan(
            const std::string& name,
            Microsoft::WRL::ComPtr<ID3D12Resource> resource,
            D3D12_RESOURCE_STATES initialState,
            const GfxResourceAllocation& allocation);

        virtual void Release(const GfxResourceAllocation& allocation) = 0;

    private:
        const D3D12_HEAP_TYPE m_HeapType;
        const D3D12_HEAP_FLAGS m_HeapFlags;
        std::queue<std::pair<uint64_t, GfxResourceAllocation>> m_ReleaseQueue;
    };

    struct GfxPlacedResourceMultiBuddyAllocatorDesc
    {
        uint32_t DefaultMaxBlockSize;
        D3D12_HEAP_TYPE HeapType;
        D3D12_HEAP_FLAGS HeapFlags;
        bool MSAA;
    };

    // 用于分配完整的 Placed Resource
    class GfxPlacedResourceMultiBuddyAllocator : public GfxCompleteResourceAllocator, protected MultiBuddyAllocator
    {
    public:
        GfxPlacedResourceMultiBuddyAllocator(GfxDevice* device, const std::string& name, const GfxPlacedResourceMultiBuddyAllocatorDesc& desc);

        GfxResourceSpan Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;
        void Release(const GfxResourceAllocation& allocation) override;

    private:
        const bool m_MSAA;
        std::vector<Microsoft::WRL::ComPtr<ID3D12Heap>> m_Heaps;
    };

    struct GfxCommittedResourceAllocatorDesc
    {
        D3D12_HEAP_TYPE HeapType;
        D3D12_HEAP_FLAGS HeapFlags;
    };

    // 用于分配完整的 Committed Resource
    class GfxCommittedResourceAllocator : public GfxCompleteResourceAllocator
    {
    public:
        GfxCommittedResourceAllocator(GfxDevice* device, const GfxCommittedResourceAllocatorDesc& desc);

        GfxResourceSpan Allocate(
            const std::string& name,
            const D3D12_RESOURCE_DESC* pDesc,
            D3D12_RESOURCE_STATES initialState,
            const D3D12_CLEAR_VALUE* pOptimizedClearValue = nullptr) override;

    protected:
        void Release(const GfxResourceAllocation& allocation) override;
    };

    class GfxBufferSubAllocator : public GfxResourceAllocator
    {
    public:
        virtual GfxResourceSpan Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) = 0;

        D3D12_HEAP_PROPERTIES GetHeapProperties() const override { return m_BufferAllocator->GetHeapProperties(); }
        D3D12_HEAP_FLAGS GetHeapFlags() const override { return m_BufferAllocator->GetHeapFlags(); }

    protected:
        GfxBufferSubAllocator(GfxCompleteResourceAllocator* bufferAllocator);

        GfxCompleteResourceAllocator* GetBufferAllocator() const { return m_BufferAllocator; }

    private:
        GfxCompleteResourceAllocator* m_BufferAllocator;
    };

    struct GfxBufferMultiBuddySubAllocatorDesc
    {
        uint32_t MinBlockSize;
        uint32_t DefaultMaxBlockSize;
        bool UnorderedAccess;
        D3D12_RESOURCE_STATES InitialResourceState;
    };

    // 用于分配 SubBuffer
    class GfxBufferMultiBuddySubAllocator : public GfxBufferSubAllocator, protected MultiBuddyAllocator
    {
    public:
        GfxBufferMultiBuddySubAllocator(const std::string& name, const GfxBufferMultiBuddySubAllocatorDesc& desc, GfxCompleteResourceAllocator* bufferAllocator);

        GfxResourceSpan Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) override;

        void DeferredRelease(const GfxResourceAllocation& allocation) override;
        void CleanUpAllocations() override;

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;

    private:
        const bool m_UnorderedAccess;
        const D3D12_RESOURCE_STATES m_InitialResourceState;

        std::vector<GfxResourceSpan> m_Buffers;
        std::queue<std::pair<uint64_t, GfxResourceAllocation>> m_ReleaseQueue;
    };

    struct GfxBufferLinearSubAllocatorDesc
    {
        uint32_t PageSize;
        bool UnorderedAccess;
        D3D12_RESOURCE_STATES InitialResourceState;
    };

    // 用于分配 SubBuffer，但只有一帧有效
    class GfxBufferLinearSubAllocator : public GfxBufferSubAllocator, protected LinearAllocator
    {
    public:
        GfxBufferLinearSubAllocator(
            const std::string& name,
            const GfxBufferLinearSubAllocatorDesc& desc,
            GfxCompleteResourceAllocator* pageAllocator,
            GfxCompleteResourceAllocator* largePageAllocator);

        GfxResourceSpan Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) override;

        void DeferredRelease(const GfxResourceAllocation& allocation) override;
        void CleanUpAllocations() override;

    protected:
        size_t RequestPage(uint32_t sizeInBytes, bool large, bool* pOutIsNew) override;

    private:
        const bool m_UnorderedAccess;
        const D3D12_RESOURCE_STATES m_InitialResourceState;
        GfxCompleteResourceAllocator* m_LargePageAllocator;

        std::vector<GfxResourceSpan> m_Pages;
        std::vector<GfxResourceSpan> m_LargePages;
        std::queue<std::pair<uint64_t, GfxResourceSpan>> m_ReleaseQueue;
    };

    // 使用类似 Unity.Collections.Allocator 的 enum 简化分配器使用

    enum class GfxAllocator
    {
        CommittedDefault, // Committed Resource, Default Heap
        CommittedUpload,  // Committed Resource, Upload Heap
        PlacedDefault,    // Placed Resource, Default Heap
        PlacedUpload,     // Placed Resource, Upload Heap
    };

    enum class GfxAllocation
    {
        Buffer,          // BUFFER
        Texture,         // NON_RT_DS_TEXTURE
        RenderTexture,   // RT_DS_TEXTURE
        RenderTextureMS, // RT_DS_TEXTURE, MSAA
    };

    enum class GfxSubAllocator
    {
        TempUpload,       // 分配 upload buffer，速度快，但生命周期只有一帧
        PersistentUpload, // 分配 upload buffer，可以持续使用
    };
}
