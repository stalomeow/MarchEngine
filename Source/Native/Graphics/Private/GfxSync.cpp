#include "GfxSync.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include <Windows.h>

namespace march
{
    GfxFence::GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue)
        : m_Value(initialValue)
    {
        GFX_HR(device->GetD3DDevice4()->CreateFence(static_cast<UINT64>(initialValue), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
        GfxUtils::SetName(m_Fence.Get(), name);

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
        return SignalNextValue([](ID3D12Fence* fence, uint64_t value)
        {
            GFX_HR(fence->Signal(static_cast<UINT64>(value)));
        });
    }

    uint64_t GfxFence::SignalNextValue(const std::function<void(ID3D12Fence*, uint64_t)>& signalFn)
    {
        uint64_t value = GetNextValue();
        signalFn(m_Fence.Get(), value);
        return (m_Value = value);
    }
}
