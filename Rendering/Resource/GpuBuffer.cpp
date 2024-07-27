#include <directx/d3dx12.h>
#include "Rendering/Resource/GpuBuffer.h"
#include "Rendering/GfxManager.h"

namespace dx12demo
{
    GpuBuffer::GpuBuffer(const std::wstring& name, UINT stride, UINT count, D3D12_HEAP_TYPE heapType)
        : m_Stride(stride), m_Count(count)
    {
        m_State = D3D12_RESOURCE_STATE_GENERIC_READ;

        auto device = GetGfxManager().GetDevice();
        THROW_IF_FAILED(device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(heapType),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(stride * count),
            m_State,
            nullptr,
            IID_PPV_ARGS(&m_Resource)));

#if defined(ENABLE_GFX_DEBUG_NAME)
        THROW_IF_FAILED(m_Resource->SetName(name.c_str()));
#else
        (name);
#endif
    }
}
