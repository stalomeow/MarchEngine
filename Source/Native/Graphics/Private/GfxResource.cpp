#include "GfxResource.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "StringUtility.h"

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
        if (m_Resource != nullptr)
        {
            m_Device->ReleaseD3D12Object(m_Resource);
            m_Resource = nullptr;
        }
    }

    void GfxResource::SetResourceName(const std::string& name)
    {
#ifdef ENABLE_GFX_DEBUG_NAME
        GFX_HR(m_Resource->SetName(StringUtility::Utf8ToUtf16(name).c_str()));
#else
        (name);
#endif
    }
}
