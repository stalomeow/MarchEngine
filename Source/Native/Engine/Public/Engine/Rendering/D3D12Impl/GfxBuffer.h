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
        Copy = 1 << 3,

        Structured = 1 << 4,
        ByteAddress = 1 << 5,

        RWStructured = 1 << 6,
        RWStructuredWithCounter = 1 << 7,
        AppendStructured = 1 << 8,
        ConsumeStructured = 1 << 9,
        RWByteAddress = 1 << 10,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxBufferUsages);

    enum class GfxBufferFlags
    {
        None = 0,
        Dynamic = 1 << 0,   // 使用 Upload Heap，优化 CPU Write
        Transient = 1 << 1, // 快速分配，但只有一帧有效；如果 Buffer 需要 Unordered Access，会忽略该标志
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxBufferFlags);

    enum class GfxBufferElement
    {
        StructuredData,
        RawData,
        StructuredCounter,
        RawCounter,
    };

    struct GfxBufferDesc
    {
        uint32_t Stride;
        uint32_t Count;
        GfxBufferUsages Usages;
        GfxBufferFlags Flags;

        bool HasAllUsages(GfxBufferUsages usages) const;

        bool HasAnyUsages(GfxBufferUsages usages) const;

        bool HasAllFlags(GfxBufferFlags flags) const;

        bool HasAnyFlags(GfxBufferFlags flags) const;

        bool HasCounter() const;

        bool AllowUnorderedAccess() const;

        bool AllowUnorderedAccess(GfxBufferElement element) const;

        uint32_t GetSizeInBytes(GfxBufferElement element) const;

        bool IsCompatibleWith(const GfxBufferDesc& other) const;
    };

    class GfxBuffer final
    {
    public:
        GfxBuffer(GfxDevice* device, const std::string& name);
        GfxBuffer(GfxDevice* device, const std::string& name, const GfxBufferDesc& desc);

        uint32_t GetOffsetInBytes(GfxBufferElement element);
        uint32_t GetSizeInBytes(GfxBufferElement element) const;

        RefCountPtr<GfxResource> GetUnderlyingResource();
        ID3D12Resource* GetUnderlyingD3DResource();

        // 为了灵活性，Buffer 不提供 CBV 和 SRV，请使用 RootCBV 和 RootSRV
        // 但 RootUav 无法使用 Counter，所以 Buffer 会提供 Uav

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(GfxBufferElement element);
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav(GfxBufferElement element);
        D3D12_VERTEX_BUFFER_VIEW GetVbv();
        D3D12_INDEX_BUFFER_VIEW GetIbv();

        // pData 可以为 nullptr
        void SetData(const void* pData, std::optional<uint32_t> counter = std::nullopt);

        // pData 可以为 nullptr
        void SetData(const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter = std::nullopt);

        void ReleaseResource();

        GfxDevice* GetDevice() const { return m_Device; }
        const std::string& GetName() const { return m_Name; }
        const GfxBufferDesc& GetDesc() const { return m_Desc; }

        ~GfxBuffer() { ReleaseResource(); }

        GfxBuffer(const GfxBuffer&) = delete;
        GfxBuffer& operator=(const GfxBuffer&) = delete;

        GfxBuffer(GfxBuffer&& other);
        GfxBuffer& operator=(GfxBuffer&& other);

    private:
        GfxDevice* m_Device;
        std::string m_Name;
        GfxBufferDesc m_Desc;

        RefCountPtr<GfxResource> m_Resource;
        uint32_t m_DataOffsetInBytes;
        uint32_t m_CounterOffsetInBytes; // 可能没有 Counter

        GfxBufferSubAllocator* m_Allocator; // 可以没有
        GfxBufferSubAllocation m_Allocation;

        // Lazy creation
        GfxOfflineDescriptor m_UavDescriptors[4];

        void AllocateResourceIfNot();
        D3D12_RANGE ReallocateResource();
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

        virtual void DeferredRelease(const GfxBufferSubAllocation& allocation) = 0;

        virtual void CleanUpAllocations() = 0;

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

        void DeferredRelease(const GfxBufferSubAllocation& allocation) override;

        void CleanUpAllocations() override;

    private:
        GfxDevice* m_Device;
        std::unique_ptr<MultiBuddyAllocator> m_Allocator;
        std::vector<RefCountPtr<GfxResource>> m_Pages;
        std::queue<std::pair<uint64_t, GfxBufferSubAllocation>> m_ReleaseQueue;
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

        void DeferredRelease(const GfxBufferSubAllocation& allocation) override;

        void CleanUpAllocations() override;

    private:
        GfxDevice* m_Device;
        std::unique_ptr<LinearAllocator> m_Allocator;
        std::vector<RefCountPtr<GfxResource>> m_Pages;
        std::vector<RefCountPtr<GfxResource>> m_LargePages;
        std::queue<std::pair<uint64_t, RefCountPtr<GfxResource>>> m_ReleaseQueue;
    };
}
