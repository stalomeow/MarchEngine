#pragma once

#include <directx/d3dx12.h>
#include "Rendering/Resource/GpuResource.h"
#include "Rendering/DxException.h"
#include "Core/MathHelper.h"
#include <string>

namespace dx12demo
{
    class GpuBuffer : public GpuResource
    {
    public:
        GpuBuffer(const std::wstring& name, UINT stride, UINT count, D3D12_HEAP_TYPE heapType);

        UINT GetStride() const { return m_Stride; }

        UINT GetCount() const { return m_Count; }

        UINT GetSize() const { return m_Stride * m_Count; }

    protected:
        UINT m_Stride;
        UINT m_Count;
    };

    class UploadBuffer : public GpuBuffer
    {
    public:
        UploadBuffer(const std::wstring& name, UINT size)
            : GpuBuffer(name, size, 1, D3D12_HEAP_TYPE_UPLOAD)
        {
            THROW_IF_FAILED(m_Resource->Map(0, nullptr, reinterpret_cast<void**>(&m_MappedData)));
        }

        ~UploadBuffer()
        {
            m_Resource->Unmap(0, nullptr);
            m_MappedData = nullptr;
        }

        BYTE* GetPointer() const { return m_MappedData; }

        UINT GetStride() const = delete;

        UINT GetCount() const = delete;

    protected:
        BYTE* m_MappedData;
    };

    const UINT ConstantBufferAlignment = 256;

    template<typename T>
    class ConstantBuffer : public GpuBuffer
    {
    public:
        ConstantBuffer(const std::wstring& name, UINT count)
            : GpuBuffer(name, MathHelper::AlignUp(sizeof(T), ConstantBufferAlignment), count, D3D12_HEAP_TYPE_UPLOAD)
        {
            CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
            THROW_IF_FAILED(m_Resource->Map(0, &readRange, reinterpret_cast<void**>(&m_MappedData)));
        }

        ~ConstantBuffer()
        {
            m_Resource->Unmap(0, nullptr);
            m_MappedData = nullptr;
        }

        void SetData(UINT index, const T& data)
        {
            memcpy(&m_MappedData[index * m_Stride], &data, sizeof(T));
        }

        static UINT CalculateStride()
        {
            // 必须是 256 的整数倍，先加 255，再去掉小于 256 的部分
            return (sizeof(T) + 255) & ~255;
        }

    protected:
        BYTE* m_MappedData;
    };

    template<typename T>
    class VertexBuffer : public GpuBuffer
    {
    public:
        VertexBuffer(const std::wstring& name, UINT count)
            : GpuBuffer(name, sizeof(T), count, D3D12_HEAP_TYPE_DEFAULT)
        {

        }

        D3D12_VERTEX_BUFFER_VIEW GetView() const
        {
            D3D12_VERTEX_BUFFER_VIEW view = {};
            view.BufferLocation = m_Resource->GetGPUVirtualAddress();
            view.SizeInBytes = GetSize();
            view.StrideInBytes = GetStride();
            return view;
        }
    };

    template<typename T>
    class IndexBuffer : public GpuBuffer
    {
        static_assert(sizeof(T) == 2 || sizeof(T) == 4, "T must be 2 or 4 bytes in size.");

    public:
        IndexBuffer(const std::wstring& name, UINT count)
            : GpuBuffer(name, sizeof(T), count, D3D12_HEAP_TYPE_DEFAULT)
        {

        }

        D3D12_INDEX_BUFFER_VIEW GetView() const
        {
            D3D12_INDEX_BUFFER_VIEW view = {};
            view.BufferLocation = m_Resource->GetGPUVirtualAddress();
            view.SizeInBytes = GetSize();
            view.Format = GetStride() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
            return view;
        }
    };
}
