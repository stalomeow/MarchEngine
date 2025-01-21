#pragma once

#include "Engine/Object.h"
#include "Engine/Allocator.h"
#include "Engine/Graphics/GfxResource.h"
#include "Engine/Graphics/GfxDescriptor.h"
#include <directx/d3dx12.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <queue>

namespace march
{
    class GfxDevice;
    class GfxBufferDataAllocator;
    class GfxBufferCounterAllocator;

    // 记录不同分配器分配的信息
    union GfxBufferDataAllocation
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
    };

    enum class GfxBufferAllocationStrategy
    {
        DefaultHeapCommitted,
        DefaultHeapPlaced,
        UploadHeapPlaced,
        UploadHeapSubAlloc,
        UploadHeapFastOneFrame,
    };

    // 为了灵活性，Buffer 不提供 CBV 和 SRV，请使用 RootCBV 和 RootSRV
    // 但 RootUav 无法使用 Counter，所以 Buffer 会提供 Uav
    class GfxBufferResource final : public ThreadSafeRefCountedObject
    {
    public:
        GfxBufferResource(
            const GfxBufferDesc& desc,
            GfxBufferDataAllocator* dataAllocator,
            const GfxBufferDataAllocation& dataAllocation,
            RefCountPtr<GfxResource> dataResource,
            uint32_t dataResourceOffsetInBytes,
            GfxBufferCounterAllocator* counterAllocator,
            RefCountPtr<GfxResource> counterResource,
            uint32_t counterResourceOffsetInBytes);

        ~GfxBufferResource();

        RefCountPtr<GfxResource> GetUnderlyingResource(GfxBufferElement element = GfxBufferElement::Data) const;
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(GfxBufferElement element = GfxBufferElement::Data) const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav();
        D3D12_VERTEX_BUFFER_VIEW GetVbv() const;
        D3D12_INDEX_BUFFER_VIEW GetIbv() const;

        GfxDevice* GetDevice() const { return m_DataResource->GetDevice(); }
        const GfxBufferDesc& GetDesc() const { return m_Desc; }

    private:
        GfxBufferDesc m_Desc;

        GfxBufferDataAllocator* m_DataAllocator;
        GfxBufferDataAllocation m_DataAllocation;
        RefCountPtr<GfxResource> m_DataResource;
        uint32_t m_DataResourceOffsetInBytes;

        // 可能没有 Counter
        GfxBufferCounterAllocator* m_CounterAllocator;
        RefCountPtr<GfxResource> m_CounterResource;
        uint32_t m_CounterResourceOffsetInBytes;

        // Lazy creation
        GfxOfflineDescriptor m_UavDescriptor;
    };

    class GfxBufferDataAllocator
    {
    public:
        virtual ~GfxBufferDataAllocator() = default;

        virtual RefCountPtr<GfxResource> Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment,
            uint32_t* pOutOffsetInBytes,
            GfxBufferDataAllocation* pOutAllocation) = 0;

        virtual void Release(const GfxBufferDataAllocation& allocation) = 0;

        bool IsHeapCpuAccessible() const { return m_BaseAllocator->IsHeapCpuAccessible(); }

    protected:
        GfxBufferDataAllocator(GfxResourceAllocator* baseAllocator) : m_BaseAllocator(baseAllocator) {}

        GfxResourceAllocator* GetBaseAllocator() const { return m_BaseAllocator; }

    private:
        GfxResourceAllocator* m_BaseAllocator;
    };

    class GfxBufferDataDirectAllocator : public GfxBufferDataAllocator
    {
    public:
        GfxBufferDataDirectAllocator(GfxResourceAllocator* bufferAllocator) : GfxBufferDataAllocator(bufferAllocator) {}

        RefCountPtr<GfxResource> Allocate(
            uint32_t sizeInBytes,
            uint32_t dataPlacementAlignment,
            uint32_t* pOutOffsetInBytes,
            GfxBufferDataAllocation* pOutAllocation) override;

        void Release(const GfxBufferDataAllocation& allocation) override {}
    };

    class GfxBufferCounterAllocator final
    {
    public:

    private:
        uint32_t m_NumElementsPerPage;
        uint32_t m_NextElementIndex;
        std::vector<RefCountPtr<GfxResource>> m_Pages;
        std::queue<std::pair<RefCountPtr<GfxResource>, uint32_t>> m_FreeElements;
    };

    class GfxBuffer final
    {
    public:
        void SetData(const GfxBufferDesc& desc, const void* src = nullptr, uint32_t counter = 0xFFFFFFFF);

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
