#pragma once

#include <directx/d3dx12.h>
#include <stdint.h>
#include <Windows.h>
#include <wrl.h>
#include <string>

namespace march
{
    class GfxDevice;
    class GfxCommandQueue;

    class GfxFence
    {
        friend GfxCommandQueue;

    public:
        GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue = 0);
        ~GfxFence();

        uint64_t GetCompletedValue() const;
        bool IsCompleted(uint64_t value) const;

        void Wait() const;
        void Wait(uint64_t value) const;
        uint64_t SignalNextValue();

        ID3D12Fence* GetD3D12Fence() const { return m_Fence.Get(); }
        uint64_t GetNextValue() const { return m_Value + 1; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        HANDLE m_EventHandle;
        uint64_t m_Value; // 上一次 signal 的值，可能从 cpu 侧 signal，也可能从 gpu 侧 signal
    };
}
