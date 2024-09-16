#pragma once

#include "GfxManager.h"
#include <directx/d3dx12.h>
#include <d3d12.h>
#include <wrl.h>

namespace march
{
    class GpuResource
    {
    public:
        virtual ~GpuResource()
        {
            if (m_Resource != nullptr)
            {
                GetGfxManager().SafeReleaseObject(m_Resource);
                m_Resource = nullptr;
            }
        }

        GpuResource(const GpuResource&) = delete;
        GpuResource& operator=(const GpuResource&) = delete;

        ID3D12Resource* GetResource() const { return m_Resource; }

        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_Resource->GetGPUVirtualAddress(); }

        D3D12_RESOURCE_STATES GetState() const { return m_State; }

        void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

        bool NeedTransition(D3D12_RESOURCE_STATES state) const { return (m_State & state) != state; }

        void ResourceBarrier(ID3D12GraphicsCommandList* cmdList, D3D12_RESOURCE_STATES stateAfter)
        {
            if (!NeedTransition(stateAfter))
            {
                return;
            }

            cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(GetResource(), GetState(), stateAfter));
            SetState(stateAfter);
        }

    protected:
        GpuResource() = default;

        ID3D12Resource* m_Resource = nullptr;
        D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;
    };
}
