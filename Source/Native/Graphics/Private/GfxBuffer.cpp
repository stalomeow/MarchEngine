#include "GfxBuffer.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "MathHelper.h"

namespace march
{
    GfxBuffer::GfxBuffer(GfxDevice* device, D3D12_HEAP_TYPE heapType, const std::string& name, uint32_t stride, uint32_t count)
        : GfxResource(device, D3D12_RESOURCE_STATE_COMMON), m_Stride(stride), m_Count(count)
    {
        UINT64 width = static_cast<UINT64>(stride) * static_cast<UINT64>(count);

        GFX_HR(device->GetD3D12Device()->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(heapType),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(width),
            m_State, nullptr, IID_PPV_ARGS(&m_Resource)));
        SetResourceName(name);
    }

    D3D12_GPU_VIRTUAL_ADDRESS GfxBuffer::GetGpuVirtualAddress(uint32_t index) const
    {
        auto offset = static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(index) * static_cast<D3D12_GPU_VIRTUAL_ADDRESS>(m_Stride);
        return m_Resource->GetGPUVirtualAddress() + offset;
    }

    GfxUploadBuffer::GfxUploadBuffer(GfxDevice* device, const std::string& name, uint32_t stride, uint32_t count, bool readable)
        : GfxBuffer(device, D3D12_HEAP_TYPE_UPLOAD, name, stride, count), m_MappedData(nullptr)
    {
        GFX_HR(m_Resource->Map(0, readable ? nullptr : &CD3DX12_RANGE(0, 0), &m_MappedData));
    }

    GfxUploadBuffer::~GfxUploadBuffer()
    {
        m_Resource->Unmap(0, nullptr);
        m_MappedData = nullptr;
    }

    uint8_t* GfxUploadBuffer::GetMappedData(uint32_t index) const
    {
        uint8_t* p = reinterpret_cast<uint8_t*>(m_MappedData);
        return &p[index * m_Stride];
    }

    GfxConstantBuffer::GfxConstantBuffer(GfxDevice* device, const std::string& name, uint32_t dataSize, uint32_t count, bool readable)
        : GfxUploadBuffer(device, name, GetAlignedSize(dataSize), count, readable)
    {
    }

    void GfxConstantBuffer::CreateView(uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE destDescriptor) const
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
        desc.BufferLocation = GetGpuVirtualAddress(index);
        desc.SizeInBytes = static_cast<UINT>(m_Stride);
        m_Device->GetD3D12Device()->CreateConstantBufferView(&desc, destDescriptor);
    }

    uint32_t GfxConstantBuffer::GetAlignedSize(uint32_t size)
    {
        return MathHelper::AlignUp(size, Alignment);
    }
}
