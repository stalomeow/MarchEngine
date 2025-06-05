#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include <Windows.h>

namespace march
{
    GfxFence::GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue)
        : m_Fence(nullptr)
        , m_EventMutex{}
        , m_NextValue(initialValue + 1)
    {
        CHECK_HR(device->GetD3DDevice4()->CreateFence(static_cast<UINT64>(initialValue), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
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

    void GfxFence::WaitOnCpu(uint64_t value)
    {
        if (m_Fence->GetCompletedValue() < static_cast<UINT64>(value))
        {
            std::lock_guard<std::mutex> lock(m_EventMutex);

            CHECK_HR(m_Fence->SetEventOnCompletion(static_cast<UINT64>(value), m_EventHandle));
            WaitForSingleObject(m_EventHandle, INFINITE);
        }
    }

    void GfxFence::WaitOnGpu(ID3D12CommandQueue* queue, uint64_t value) const
    {
        CHECK_HR(queue->Wait(m_Fence.Get(), static_cast<UINT64>(value)));
    }

    uint64_t GfxFence::SignalNextValueOnCpu()
    {
        uint64_t value = m_NextValue.fetch_add(1, std::memory_order_relaxed);
        CHECK_HR(m_Fence->Signal(static_cast<UINT64>(value)));
        return value;
    }

    uint64_t GfxFence::SignalNextValueOnGpu(ID3D12CommandQueue* queue)
    {
        uint64_t value = m_NextValue.fetch_add(1, std::memory_order_relaxed);
        CHECK_HR(queue->Signal(m_Fence.Get(), static_cast<UINT64>(value)));
        return value;
    }

    uint64_t GfxFence::GetNextValue() const
    {
        return m_NextValue.load(std::memory_order_relaxed);
    }
}
