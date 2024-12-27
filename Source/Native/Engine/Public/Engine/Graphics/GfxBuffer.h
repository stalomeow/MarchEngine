#pragma once

#include "Engine/Graphics/GfxResource.h"
#include <directx/d3dx12.h>
#include <string>
#include <stdint.h>

namespace march
{
    class GfxDevice;

    struct GfxBufferDesc
    {
        uint32_t SizeInBytes;
        bool UnorderedAccess;
        D3D12_RESOURCE_STATES InitialState;
    };

    // 为了灵活性，Buffer 不提供 CBV 和 SRV，请使用 RootCBV 和 RootSRV
    // 但 RootUav 无法使用 Counter，所以 Buffer 会提供 Uav
    class GfxBuffer
    {
    public:
        GfxBuffer();
        GfxBuffer(GfxDevice* device, const std::string& name, const GfxBufferDesc& desc, GfxAllocator allocator);
        GfxBuffer(GfxDevice* device, uint32_t sizeInBytes, uint32_t dataPlacementAlignment, GfxSubAllocator allocator);
        virtual ~GfxBuffer() { Release(); }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t offset = 0) const;
        void SetData(uint32_t destOffset, const void* src, uint32_t sizeInBytes);

        uint32_t GetResourceOffset() const { return m_Resource.GetBufferOffset(); }
        uint32_t GetSize() const { return m_Resource.GetBufferSize(); }
        std::shared_ptr<GfxResource> GetResource() const { return m_Resource.GetResource(); }
        GfxDevice* GetDevice() const { return m_Resource.GetDevice(); }

        GfxBuffer(const GfxBuffer&) = delete;
        GfxBuffer& operator=(const GfxBuffer&) = delete;

        GfxBuffer(GfxBuffer&&) noexcept;
        GfxBuffer& operator=(GfxBuffer&&);

    private:
        GfxResourceSpan m_Resource;
        uint8_t* m_MappedData;

        void Release();
        void TryMapData(GfxResourceAllocator* allocator);
    };

    template <typename T>
    class GfxVertexBuffer : public GfxBuffer
    {
    public:
        GfxVertexBuffer() : GfxBuffer() {}

        GfxVertexBuffer(GfxDevice* device, const std::string& name, uint32_t count, GfxAllocator allocator)
            : GfxBuffer(device, name, GfxBufferDesc{ static_cast<uint32_t>(sizeof(T)) * count, false, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER }, allocator)
        {
        }

        // TODO 我不清楚 vertex buffer 有没有 dataPlacementAlignment

        GfxVertexBuffer(GfxDevice* device, uint32_t count, GfxSubAllocator allocator)
            : GfxBuffer(device, static_cast<uint32_t>(sizeof(T)) * count, /* dataPlacementAlignment */ 0, allocator)
        {
        }

        D3D12_VERTEX_BUFFER_VIEW GetVbv() const
        {
            D3D12_VERTEX_BUFFER_VIEW view{};
            view.BufferLocation = GetGpuVirtualAddress();
            view.SizeInBytes = static_cast<UINT>(GetSize());
            view.StrideInBytes = sizeof(T);
            return view;
        }
    };

    class GfxIndexBufferUInt16 : public GfxBuffer
    {
    public:
        GfxIndexBufferUInt16() : GfxBuffer() {}

        GfxIndexBufferUInt16(GfxDevice* device, const std::string& name, uint32_t count, GfxAllocator allocator)
            : GfxBuffer(device, name, GfxBufferDesc{ static_cast<uint32_t>(sizeof(uint16_t)) * count, false, D3D12_RESOURCE_STATE_INDEX_BUFFER }, allocator)
        {
        }

        // TODO 我不清楚 index buffer 有没有 dataPlacementAlignment

        GfxIndexBufferUInt16(GfxDevice* device, uint32_t count, GfxSubAllocator allocator)
            : GfxBuffer(device, static_cast<uint32_t>(sizeof(uint16_t)) * count, /* dataPlacementAlignment */ 0, allocator)
        {
        }

        D3D12_INDEX_BUFFER_VIEW GetIbv() const
        {
            D3D12_INDEX_BUFFER_VIEW view{};
            view.BufferLocation = GetGpuVirtualAddress();
            view.SizeInBytes = static_cast<UINT>(GetSize());
            view.Format = DXGI_FORMAT_R16_UINT;
            return view;
        }
    };

    class GfxIndexBufferUInt32 : public GfxBuffer
    {
    public:
        GfxIndexBufferUInt32() : GfxBuffer() {}

        GfxIndexBufferUInt32(GfxDevice* device, const std::string& name, uint32_t count, GfxAllocator allocator)
            : GfxBuffer(device, name, GfxBufferDesc{ static_cast<uint32_t>(sizeof(uint32_t)) * count, false, D3D12_RESOURCE_STATE_INDEX_BUFFER }, allocator)
        {
        }

        // TODO 我不清楚 index buffer 有没有 dataPlacementAlignment

        GfxIndexBufferUInt32(GfxDevice* device, uint32_t count, GfxSubAllocator allocator)
            : GfxBuffer(device, static_cast<uint32_t>(sizeof(uint32_t)) * count, /* dataPlacementAlignment */ 0, allocator)
        {
        }

        D3D12_INDEX_BUFFER_VIEW GetIbv() const
        {
            D3D12_INDEX_BUFFER_VIEW view{};
            view.BufferLocation = GetGpuVirtualAddress();
            view.SizeInBytes = static_cast<UINT>(GetSize());
            view.Format = DXGI_FORMAT_R32_UINT;
            return view;
        }
    };

    template <typename T>
    class GfxConstantBuffer : public GfxBuffer
    {
    public:
        GfxConstantBuffer() : GfxBuffer() {}

        GfxConstantBuffer(GfxDevice* device, const std::string& name, GfxAllocator allocator)
            : GfxBuffer(device, name, GfxBufferDesc{ static_cast<uint32_t>(sizeof(T)), false, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER }, allocator)
        {
        }

        GfxConstantBuffer(GfxDevice* device, GfxSubAllocator allocator)
            : GfxBuffer(device, static_cast<uint32_t>(sizeof(T)), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, allocator)
        {
        }
    };

    class GfxRawConstantBuffer : public GfxBuffer
    {
    public:
        GfxRawConstantBuffer() : GfxBuffer() {}

        GfxRawConstantBuffer(GfxDevice* device, const std::string& name, uint32_t sizeInBytes, GfxAllocator allocator)
            : GfxBuffer(device, name, GfxBufferDesc{ sizeInBytes, false, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER }, allocator)
        {
        }

        GfxRawConstantBuffer(GfxDevice* device, uint32_t sizeInBytes, GfxSubAllocator allocator)
            : GfxBuffer(device, sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, allocator)
        {
        }
    };

    template <typename T>
    class GfxStructuredBuffer : public GfxBuffer
    {
    public:
        GfxStructuredBuffer() : GfxBuffer() {}

        GfxStructuredBuffer(GfxDevice* device, const std::string& name, uint32_t count, GfxAllocator allocator)
            : GfxBuffer(device, name, GfxBufferDesc{ static_cast<uint32_t>(sizeof(T)) * count, false, D3D12_RESOURCE_STATE_GENERIC_READ }, allocator)
        {
        }

        // TODO 我不清楚 structured buffer 有没有 dataPlacementAlignment

        GfxStructuredBuffer(GfxDevice* device, uint32_t count, GfxSubAllocator allocator)
            : GfxBuffer(device, static_cast<uint32_t>(sizeof(T)) * count, /* dataPlacementAlignment */ 0, allocator)
        {
        }
    };
}
