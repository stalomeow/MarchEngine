#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include <deque>
#include <queue>
#include <memory>

namespace march
{
    class GfxDevice;
    class GfxUploadBuffer;

    class GfxUploadMemory
    {
    public:
        GfxUploadMemory(GfxUploadBuffer* buffer, uint32_t offset, uint32_t stride, uint32_t count);

        uint8_t* GetMappedData(uint32_t index) const;
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t index) const;
        ID3D12Resource* GetResource() const;

        uint32_t GetStride() const { return m_Stride; }
        uint32_t GetCount() const { return m_Count; }
        uint32_t GetSize() const { return m_Stride * m_Count; }

    private:
        GfxUploadBuffer* m_Buffer;
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
        static const uint32_t PageSize = 4096;

    private:
        GfxDevice* m_Device;

        uint32_t m_AllocateOffset;
        uint32_t m_PageCounter; // 统计已经分配的 page 数量（不包括 large page）
        std::deque<std::unique_ptr<GfxUploadBuffer>> m_UsedPages;
        std::queue<std::pair<uint64_t, std::unique_ptr<GfxUploadBuffer>>> m_ReleaseQueue;
    };
}
