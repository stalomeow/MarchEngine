#pragma once

#include "Allocator.h"
#include <directx/d3dx12.h>
#include <string>
#include <stdint.h>
#include <vector>
#include <queue>
#include <memory>
#include <Windows.h>

namespace march
{
    class GfxDevice;
    class GfxResource;
    class GfxResourceAllocator;

    enum class GfxBufferUsage
    {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Constant = 1 << 2,
        Structured = 1 << 3,
        Raw = 1 << 4,
        Counter = 1 << 5,
        UnorderedAccess = 1 << 6,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxBufferUsage);

    struct GfxBufferDesc
    {
        uint32_t Stride;
        uint32_t Count;
        D3D12_RESOURCE_STATES InitialState;
    };

    class GfxBuffer
    {
    public:
        virtual ~GfxBuffer();

        virtual GfxResource* GetBufferResource() const = 0;
        virtual uint32_t GetBufferResourceOffset() const = 0;

        virtual GfxResource* GetCounterResource() const = 0;
        virtual uint32_t GetCounterResourceOffset() const = 0;

        uint32_t GetStride() const { return m_Stride; }
        uint32_t GetCount() const { return m_Count; }
        uint32_t GetSize() const { return m_Stride * m_Count; }

    protected:
        GfxBuffer(uint32_t stride, uint32_t count);

    private:
        const uint32_t m_Stride;
        const uint32_t m_Count;
    };

    class GfxAABuffer
    {
    public:
        GfxAABuffer(const std::string& name, const GfxBufferDesc& desc, GfxResourceAllocator* allocator);
        virtual ~GfxAABuffer();

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t index) const;
        void* GetDataPointer(uint32_t index) const;

        D3D12_VERTEX_BUFFER_VIEW GetVbv() const;
        D3D12_INDEX_BUFFER_VIEW GetIbv() const;
        D3D12_CPU_DESCRIPTOR_HANDLE GetSrv();
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav();

        template <typename T>
        T& GetData(uint32_t index) const
        {
            static_assert(sizeof(T) <= m_Stride, "Size of T is larger than stride");
            return *static_cast<T*>(GetDataPointer(index));
        }

    private:
        const uint32_t m_Stride;
        const uint32_t m_Count;
        std::unique_ptr<GfxResource> m_BufferResource;
        std::unique_ptr<GfxResource> m_CounterResource;
        void* m_MappedData;
    };

    // 考虑 Buffer SRV，还必须按照 stride 对齐
    // D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT

    class GfxUploadMemory
    {
    public:
        GfxUploadMemory(GfxBuffer* buffer, uint32_t offset, uint32_t stride, uint32_t count);

        uint8_t* GetMappedData(uint32_t index) const;
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t index) const;
        uint32_t GetD3D12ResourceOffset(uint32_t index) const;
        ID3D12Resource* GetD3D12Resource() const;

        uint32_t GetStride() const { return m_Stride; }
        uint32_t GetCount() const { return m_Count; }
        uint32_t GetSize() const { return m_Stride * m_Count; }

    private:
        GfxBuffer* m_Buffer;
        uint32_t m_Offset;
        uint32_t m_Stride;
        uint32_t m_Count;
    };

    class GfxUploadMemoryAllocator
    {
    public:
        GfxUploadMemoryAllocator(GfxDevice* device);
        ~GfxUploadMemoryAllocator() = default;

        GfxUploadMemoryAllocator(const GfxUploadMemoryAllocator&) = delete;
        GfxUploadMemoryAllocator& operator=(const GfxUploadMemoryAllocator&) = delete;

        void BeginFrame();
        void EndFrame(uint64_t fenceValue);
        GfxUploadMemory Allocate(uint32_t size, uint32_t count = 1, uint32_t alignment = 1);

    public:
        static const uint32_t PageSize = 4 * 1024 * 1024; // 4MB

    private:
        GfxDevice* m_Device;

        uint32_t m_AllocateOffset;
        uint32_t m_PageCounter; // 统计已经分配的 page 数量（不包括 large page）
        std::vector<std::unique_ptr<GfxUploadBuffer>> m_UsedPages;
        std::vector<std::unique_ptr<GfxUploadBuffer>> m_LargePages;
        std::queue<std::pair<uint64_t, std::unique_ptr<GfxUploadBuffer>>> m_ReleaseQueue;
    };

    struct GfxSubBufferMultiBuddyAllocatorDesc
    {
        uint32_t MinBlockSize;
        uint32_t DefaultMaxBlockSize;
        D3D12_RESOURCE_FLAGS ResourceFlags;
        D3D12_RESOURCE_STATES InitialResourceState;
    };

    class GfxSubBufferMultiBuddyAllocator : protected MultiBuddyAllocator
    {
    public:
        GfxSubBufferMultiBuddyAllocator(const std::string& name, const GfxSubBufferMultiBuddyAllocatorDesc& desc, GfxResourceAllocator* bufferAllocator);

    protected:
        void AppendNewAllocator(uint32_t maxBlockSize) override;

    private:
        const D3D12_RESOURCE_FLAGS m_ResourceFlags;
        const D3D12_RESOURCE_STATES m_InitialResourceState;
        GfxResourceAllocator* m_BufferAllocator;
        std::vector<std::unique_ptr<GfxBuffer>> m_Buffers;
    };

    class GfxSubBufferLinearAllocator : protected LinearAllocator
    {
    private:
        const D3D12_RESOURCE_FLAGS m_ResourceFlags;
        const D3D12_RESOURCE_STATES m_InitialResourceState;
        GfxResourceAllocator* m_BufferAllocator;
    };
}
