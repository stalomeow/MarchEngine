#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>

namespace march
{
    class GfxDevice;
    class GfxFence;
    enum class GfxCommandListType;

    class GfxCommandQueue
    {
    public:
        GfxCommandQueue(GfxDevice* device, GfxCommandListType type, const std::string& name, int32_t priority = 0, bool disableGpuTimeout = false);
        ~GfxCommandQueue() = default;

        GfxCommandListType GetType() const;
        ID3D12CommandQueue* GetD3D12CommandQueue() const { return m_CommandQueue.Get(); }

        void Wait(GfxFence* fence);
        void Wait(GfxFence* fence, uint64_t value);
        uint64_t SignalNextValue(GfxFence* fence);

    private:
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
    };
}
