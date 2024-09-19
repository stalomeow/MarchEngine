#pragma once

#include <directx/d3dx12.h>

namespace march
{
    class GfxDevice;

    class GfxResource
    {
    protected:
        GfxResource(GfxDevice* device, D3D12_RESOURCE_STATES state);
        void SetD3D12ResourceName(const std::string& name);
        void ReleaseD3D12Resource();

    public:
        GfxResource(const GfxResource&) = delete;
        GfxResource& operator=(const GfxResource&) = delete;

        virtual ~GfxResource();

        bool NeedStateTransition(D3D12_RESOURCE_STATES stateAfter) const;

        GfxDevice* GetDevice() const { return m_Device; }
        ID3D12Resource* GetD3D12Resource() const { return m_Resource; }
        D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_Resource->GetGPUVirtualAddress(); }
        D3D12_RESOURCE_STATES GetState() const { return m_State; }
        void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

    protected:
        GfxDevice* m_Device;
        ID3D12Resource* m_Resource;
        D3D12_RESOURCE_STATES m_State;
    };
}
