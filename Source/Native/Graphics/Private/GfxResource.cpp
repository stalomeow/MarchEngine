#include "GfxResource.h"
#include "GfxDevice.h"
#include "StringUtils.h"

namespace march
{
    GfxResource::GfxResource(GfxDevice* device, D3D12_RESOURCE_STATES state)
        : m_Device(device)
        , m_Resource(nullptr)
        , m_State(state)
    {
    }

    GfxResource::~GfxResource()
    {
        ReleaseD3D12Resource();
    }

    void GfxResource::SetD3D12ResourceName(const std::string& name)
    {
#ifdef ENABLE_GFX_DEBUG_NAME
        GFX_HR(m_Resource->SetName(StringUtils::Utf8ToUtf16(name).c_str()));
#else
        (name);
#endif
    }

    void GfxResource::ReleaseD3D12Resource()
    {
        if (m_Resource != nullptr)
        {
            m_Device->ReleaseD3D12Object(m_Resource);
            m_Resource = nullptr;
        }
    }

    bool GfxResource::NeedStateTransition(D3D12_RESOURCE_STATES stateAfter) const
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states
        // D3D12_RESOURCE_STATE_COMMON 为 0，要特殊处理

        if (stateAfter == D3D12_RESOURCE_STATE_COMMON)
        {
            return m_State != stateAfter;
        }

        return (m_State & stateAfter) != stateAfter;
    }
}
