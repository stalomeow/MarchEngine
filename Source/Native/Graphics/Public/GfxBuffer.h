#pragma once

#include "GfxResource.h"
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

    class GfxBuffer : public GfxResource
    {
    public:
        ~GfxBuffer() override;

        D3D12_CPU_DESCRIPTOR_HANDLE GetSrv() override;
        D3D12_CPU_DESCRIPTOR_HANDLE GetUav() override;

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint64_t index) const;

        uint64_t GetStride() const { return m_Stride; }
        uint64_t GetCount() const { return m_Count; }
        uint64_t GetSize() const { return m_Stride * m_Count; }

    protected:
        GfxBuffer(GfxDevice* device, const std::string& name, D3D12_HEAP_TYPE type, uint64_t stride, uint64_t count, bool unorderedAccess);

        uint64_t m_Stride;
        uint64_t m_Count;
    };

    class GfxUploadBuffer : public GfxBuffer
    {
    public:
        GfxUploadBuffer(GfxDevice* device, const std::string& name, uint32_t stride, uint32_t count, bool readable);
        virtual ~GfxUploadBuffer();

        uint8_t* GetMappedData(uint32_t index) const;

    protected:
        void* m_MappedData;
    };

    class GfxConstantBuffer : public GfxUploadBuffer
    {
    public:
        GfxConstantBuffer(GfxDevice* device, const std::string& name, uint32_t dataSize, uint32_t count, bool readable);
        virtual ~GfxConstantBuffer() = default;

        void CreateView(uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) const;

        static constexpr uint32_t Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        static uint32_t GetAlignedSize(uint32_t size);
    };

    template<typename T>
    class GfxVertexBuffer : public GfxBuffer
    {
    public:
        GfxVertexBuffer(GfxDevice* device, const std::string& name, uint32_t count)
            : GfxBuffer(device, D3D12_HEAP_TYPE_DEFAULT, name, sizeof(T), count)
        {
        }

        virtual ~GfxVertexBuffer() = default;

        D3D12_VERTEX_BUFFER_VIEW GetView() const
        {
            D3D12_VERTEX_BUFFER_VIEW view = {};
            view.BufferLocation = m_Resource->GetGPUVirtualAddress();
            view.SizeInBytes = static_cast<UINT>(GetSize());
            view.StrideInBytes = static_cast<UINT>(GetStride());
            return view;
        }
    };

    template<typename T>
    class GfxIndexBuffer : public GfxBuffer
    {
        static_assert(sizeof(T) == 2 || sizeof(T) == 4, "T must be 2 or 4 bytes in size.");

    public:
        GfxIndexBuffer(GfxDevice* device, const std::string& name, uint32_t count)
            : GfxBuffer(device, D3D12_HEAP_TYPE_DEFAULT, name, sizeof(T), count)
        {
        }

        virtual ~GfxIndexBuffer() = default;

        D3D12_INDEX_BUFFER_VIEW GetView() const
        {
            D3D12_INDEX_BUFFER_VIEW view = {};
            view.BufferLocation = m_Resource->GetGPUVirtualAddress();
            view.SizeInBytes = static_cast<UINT>(GetSize());
            view.Format = GetStride() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            return view;
        }
    };

    class GfxUploadMemory
    {
    public:
        GfxUploadMemory(GfxUploadBuffer* buffer, uint32_t offset, uint32_t stride, uint32_t count);

        uint8_t* GetMappedData(uint32_t index) const;
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t index) const;
        uint32_t GetD3D12ResourceOffset(uint32_t index) const;
        ID3D12Resource* GetD3D12Resource() const;

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
        static const uint32_t PageSize = 4 * 1024 * 1024; // 4MB

    private:
        GfxDevice* m_Device;

        uint32_t m_AllocateOffset;
        uint32_t m_PageCounter; // 统计已经分配的 page 数量（不包括 large page）
        std::vector<std::unique_ptr<GfxUploadBuffer>> m_UsedPages;
        std::vector<std::unique_ptr<GfxUploadBuffer>> m_LargePages;
        std::queue<std::pair<uint64_t, std::unique_ptr<GfxUploadBuffer>>> m_ReleaseQueue;
    };
}
