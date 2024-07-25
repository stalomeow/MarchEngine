#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include <wrl.h>
#include "Rendering/DxException.h"

namespace dx12demo
{
    enum class UploadBufferType
    {
        Constant,
    };

    template<typename T>
    class UploadBuffer
    {
    public:
        UploadBuffer(ID3D12Device* device, UploadBufferType type, UINT count)
            : m_Type(type), m_Stride(CalculateStride(type, sizeof(T))), m_Count(count)
        {
            THROW_IF_FAILED(device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(m_Stride * m_Count),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_Buffer)));

            if (IsPermanentlyMapped())
            {
                // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12resource-map
                // Map the constant buffers. Note that unlike D3D11, the resource
                // does not need to be unmapped for use by the GPU. In this sample,
                // the resource stays 'permanently' mapped to avoid overhead with
                // mapping/unmapping each frame.
                Map();

                // When using persistent map, the application must ensure the CPU finishes writing data into memory before
                // the GPU executes a command list that reads or writes the memory.
                // In common scenarios, the application merely must write to memory before calling ExecuteCommandLists;
                // but using a fence to delay command list execution works as well.
            }
        }
        UploadBuffer(const UploadBuffer& rhs) = delete;
        UploadBuffer& operator=(const UploadBuffer& rhs) = delete;

        ~UploadBuffer()
        {
            if (m_Buffer != nullptr && IsPermanentlyMapped())
            {
                Unmap();
            }
        }

        UINT GetStride() const { return m_Stride; }
        UINT GetCount() const { return m_Count; }
        ID3D12Resource* GetResource() const { return m_Buffer.Get(); }

        void SetData(UINT index, const T& data)
        {
            if (IsPermanentlyMapped())
            {
                memcpy(&m_MappedData[index * m_Stride], &data, sizeof(T));
            }
            else
            {
                Map();
                memcpy(&m_MappedData[index * m_Stride], &data, sizeof(T));
                Unmap();
            }
        }

    private:
        void Map()
        {
            CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.
            THROW_IF_FAILED(m_Buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_MappedData)));
        }

        void Unmap()
        {
            m_Buffer->Unmap(0, nullptr);
            m_MappedData = nullptr;
        }

        bool IsPermanentlyMapped() const { return m_Type == UploadBufferType::Constant; }

        static UINT CalculateStride(UploadBufferType type, int elementSize)
        {
            if (type == UploadBufferType::Constant)
            {
                // 必须是 256 的整数倍
                // 先加 255，再去掉小于 256 的部分
                return (elementSize + 255) & ~255;
            }

            return elementSize;
        }

    private:
        UploadBufferType m_Type;
        UINT m_Stride;
        UINT m_Count;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer = nullptr;
        BYTE* m_MappedData = nullptr;
    };
}
