#pragma once

#include "Engine/Object.h"
#include "Engine/Memory/Allocator.h"
#include "Engine/Graphics/GfxResource.h"
#include "Engine/Graphics/GfxDescriptor.h"
#include <directx/d3dx12.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <queue>
#include <memory>

namespace march
{
    class GfxDevice;
    class GfxBufferSubAllocator;

    // 记录不同分配器分配的信息
    union GfxBufferSubAllocation
    {
        BuddyAllocation Buddy;
    };

    enum class GfxBufferUsages
    {
        Vertex = 1 << 0,
        Index = 1 << 1,
        Constant = 1 << 2,
        Structured = 1 << 3,
        Raw = 1 << 4,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxBufferUsages);

    enum class GfxBufferUnorderedAccessMode
    {
        // Disable Unordered Access
        Disabled,

        // RWStructuredBuffer without IncrementCounter and DecrementCounter functions
        Structured,

        // AppendStructuredBuffer, ConsumeStructuredBuffer, RWStructuredBuffer
        StructuredWithCounter,

        // RWByteAddressBuffer
        Raw,
    };

    enum class GfxBufferElement
    {
        Data,
        Counter,
    };

    struct GfxBufferDesc
    {
        uint32_t Stride;
        uint32_t Count;
        GfxBufferUsages Usages;
        GfxBufferUnorderedAccessMode UnorderedAccessMode;

        uint32_t GetSizeInBytes() const { return Stride * Count; }
        bool HasCounter() const { return UnorderedAccessMode == GfxBufferUnorderedAccessMode::StructuredWithCounter; }
    };

    enum class GfxBufferAllocationStrategy
    {
        DefaultHeapCommitted,
        DefaultHeapPlaced,
        UploadHeapPlaced,
        UploadHeapSubAlloc,
        UploadHeapFastOneFrame,
    };

    class GfxBufferResource final : public ThreadSafeRefCountedObject
    {
    public:
        GfxBufferResource(
            const GfxBufferDesc& desc,
            RefCountPtr<GfxResource> resource,
            uint32_t dataOffsetInBytes,
            uint32_t counterOffsetInBytes);

        GfxBufferResource(
            const GfxBufferDesc& desc,
            GfxBufferSubAllocator* allocator,
            const GfxBufferSubAllocation& allocation,
            RefCountPtr<GfxResource> resource,
            uint32_t dataOffsetInBytes,
            uint32_t counterOffsetInBytes);

        ~GfxBufferResource();

        // 为了灵活性，Buffer 不提供 CBV 和 SRV，请使用 RootCBV 和 RootSRV
        // 但 RootUav 无法使用 Counter，所以 Buffer 会提供 Uav

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(GfxBufferElement element = GfxBufferElement::Data) const;
        uint32_t GetOffsetInBytes(GfxBufferElement element = GfxBufferElement::Data) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav();
        D3D12_VERTEX_BUFFER_VIEW GetVbv() const;
        D3D12_INDEX_BUFFER_VIEW GetIbv() const;

        GfxDevice* GetDevice() const { return m_Resource->GetDevice(); }
        RefCountPtr<GfxResource> GetUnderlyingResource() const { return m_Resource; }
        GfxBufferSubAllocator* GetAllocator() const { return m_Allocator; }
        const GfxBufferDesc& GetDesc() const { return m_Desc; }

    private:
        GfxBufferDesc m_Desc;

        RefCountPtr<GfxResource> m_Resource;
        uint32_t m_DataOffsetInBytes;
        uint32_t m_CounterOffsetInBytes; // 可能没有 Counter

        GfxBufferSubAllocator* m_Allocator; // 可以没有
        GfxBufferSubAllocation m_Allocation;

        // Lazy creation
        GfxOfflineDescriptor m_UavDescriptor;
    };

    class GfxBufferSubAllocator
    {
    public:
        virtual ~GfxBufferSubAllocator() = default;

        virtual RefCountPtr<GfxResource> Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment,
            uint32_t* pOutOffsetInBytes,
            GfxBufferSubAllocation* pOutAllocation) = 0;

        virtual void Release(const GfxBufferSubAllocation& allocation) = 0;

        virtual void CleanUpAllocations() {}

    protected:
        GfxBufferSubAllocator() = default;
    };

    struct GfxBufferMultiBuddySubAllocatorDesc
    {
        uint32_t MinBlockSize;
        uint32_t DefaultMaxBlockSize;
    };

    class GfxBufferMultiBuddySubAllocator : public GfxBufferSubAllocator
    {
    public:
        GfxBufferMultiBuddySubAllocator(
            const std::string& name,
            const GfxBufferMultiBuddySubAllocatorDesc& desc,
            GfxResourceAllocator* pageAllocator);

        RefCountPtr<GfxResource> Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment,
            uint32_t* pOutOffsetInBytes,
            GfxBufferSubAllocation* pOutAllocation) override;

        void Release(const GfxBufferSubAllocation& allocation) override;

    private:
        std::unique_ptr<MultiBuddyAllocator> m_Allocator;
        std::vector<RefCountPtr<GfxResource>> m_Pages;
    };

    struct GfxBufferLinearSubAllocatorDesc
    {
        uint32_t PageSize;
    };

    // 分配结果只有一帧有效
    class GfxBufferLinearSubAllocator : public GfxBufferSubAllocator
    {
    public:
        GfxBufferLinearSubAllocator(
            const std::string& name,
            const GfxBufferLinearSubAllocatorDesc& desc,
            GfxResourceAllocator* pageAllocator,
            GfxResourceAllocator* largePageAllocator);

        RefCountPtr<GfxResource> Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment,
            uint32_t* pOutOffsetInBytes,
            GfxBufferSubAllocation* pOutAllocation) override;

        void Release(const GfxBufferSubAllocation& allocation) override;

        void CleanUpAllocations() override;

    private:
        GfxDevice* m_Device;
        std::unique_ptr<LinearAllocator> m_Allocator;
        std::vector<RefCountPtr<GfxResource>> m_Pages;
        std::vector<RefCountPtr<GfxResource>> m_LargePages;
        std::queue<std::pair<uint64_t, RefCountPtr<GfxResource>>> m_ReleaseQueue;
    };

    class GfxBuffer final
    {
    public:
        static constexpr uint32_t NullCounter = 0xFFFFFFFF;

        void SetData(
            const GfxBufferDesc& desc,
            GfxBufferAllocationStrategy allocationStrategy,
            const void* pData = nullptr,
            uint32_t counter = NullCounter);

        RefCountPtr<GfxBufferResource> GetResource() const { return m_Resource; }

        GfxBuffer() = default;
        ~GfxBuffer() = default;

        GfxBuffer(const GfxBuffer&) = default;
        GfxBuffer& operator=(const GfxBuffer&) = default;

        GfxBuffer(GfxBuffer&&) = default;
        GfxBuffer& operator=(GfxBuffer&&) = default;

    private:
        RefCountPtr<GfxBufferResource> m_Resource;
    };
}
