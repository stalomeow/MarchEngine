#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <string>
#include <stdint.h>
#include <functional>

namespace march
{
    class GfxDevice;

    class GfxFence final
    {
    public:
        GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue = 0);
        ~GfxFence();

        uint64_t GetCompletedValue() const;
        bool IsCompleted(uint64_t value) const;
        void Wait(uint64_t value) const;

        uint64_t SignalNextValue();
        uint64_t SignalNextValue(const std::function<void(ID3D12Fence*, uint64_t)>& signalFn);
        uint64_t GetNextValue() const { return m_Value + 1; }

        ID3D12Fence* GetFence() const { return m_Fence.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        HANDLE m_EventHandle;
        uint64_t m_Value; // 上一次 signal 的值，可能从 cpu 侧 signal，也可能从 gpu 侧 signal
    };

    class GfxSyncPoint final
    {
    public:
        GfxSyncPoint(GfxFence* fence, uint64_t value) : m_Fence(fence), m_Value(value) {}

        GfxFence* GetFence() const { return m_Fence; }
        uint64_t GetValue() const { return m_Value; }

        void Wait() const { m_Fence->Wait(m_Value); }
        bool IsCompleted() const { return m_Fence->IsCompleted(m_Value); }

        GfxSyncPoint(const GfxSyncPoint&) = default;
        GfxSyncPoint& operator=(const GfxSyncPoint&) = default;

        GfxSyncPoint(GfxSyncPoint&&) = default;
        GfxSyncPoint& operator=(GfxSyncPoint&&) = default;

    private:
        GfxFence* m_Fence;
        uint64_t m_Value;
    };
}
