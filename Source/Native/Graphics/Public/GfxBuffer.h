#pragma once

#include "GfxResource.h"
#include <directx/d3dx12.h>
#include <string>
#include <stdint.h>
#include <Windows.h>

namespace march
{
    class GfxDevice;

    class GfxBuffer : public GfxResource
    {
    protected:
        GfxBuffer(GfxDevice* device, D3D12_HEAP_TYPE heapType, const std::string& name, uint32_t stride, uint32_t count);

    public:
        virtual ~GfxBuffer() = default;

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress(uint32_t index) const;

        uint32_t GetStride() const { return m_Stride; }
        uint32_t GetCount() const { return m_Count; }
        uint32_t GetSize() const { return m_Stride * m_Count; }

    protected:
        uint32_t m_Stride;
        uint32_t m_Count;
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

        static const uint32_t Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        static uint32_t GetAlignedSize(uint32_t size);
    };

    template<typename T>
    class GfxVertexBuffer : public GfxBuffer
    {
    public:
        GfxVertexBuffer(GfxDevice* device, const std::wstring& name, UINT count)
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
        GfxIndexBuffer(GfxDevice* device, const std::wstring& name, UINT count)
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
}
