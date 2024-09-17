#include "GfxFence.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "StringUtility.h"

namespace march
{
    GfxFence::GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue) : m_Value(initialValue)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        ID3D12Device4* d3d12Device = device->GetD3D12Device();
        GFX_HR(d3d12Device->CreateFence(static_cast<UINT64>(initialValue), flags, IID_PPV_ARGS(&m_Fence)));

#ifdef ENABLE_GFX_DEBUG_NAME
        GFX_HR(m_Fence->SetName(StringUtility::Utf8ToUtf16(name).c_str()));
#else
        (name);
#endif

        m_EventHandle = CreateEventExW(NULL, NULL, NULL, EVENT_ALL_ACCESS);
    }

    GfxFence::~GfxFence()
    {
        CloseHandle(m_EventHandle);
    }

    uint64_t GfxFence::GetCompletedValue() const
    {
        return static_cast<uint64_t>(m_Fence->GetCompletedValue());
    }

    bool GfxFence::IsCompleted(uint64_t value) const
    {
        return static_cast<UINT64>(value) <= m_Fence->GetCompletedValue();
    }

    void GfxFence::Wait() const
    {
        Wait(m_Value);
    }

    void GfxFence::Wait(uint64_t value) const
    {
        if (m_Fence->GetCompletedValue() < static_cast<UINT64>(value))
        {
            GFX_HR(m_Fence->SetEventOnCompletion(static_cast<UINT64>(value), m_EventHandle));
            WaitForSingleObject(m_EventHandle, INFINITE);
        }
    }

    uint64_t GfxFence::SignalNextValue()
    {
        uint64_t value = ++m_Value;
        GFX_HR(m_Fence->Signal(static_cast<UINT64>(value)));
        return value;
    }
}
