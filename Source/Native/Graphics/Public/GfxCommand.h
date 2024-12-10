#pragma once

#include "GfxSync.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdint.h>
#include <memory>

namespace march
{
    class GfxDevice;
    class GfxResource;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    enum class GfxCommandType
    {
        Direct, // 3D rendering engine
        Compute,
        Copy,
        NumTypes,
    };

    class GfxCommandQueue final
    {
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        GfxCommandQueue(GfxDevice* device, const std::string& name, GfxCommandType type, int32_t priority = 0, bool disableGpuTimeout = false);

        GfxCommandType GetType() const { return m_Type; }
        ID3D12CommandQueue* GetQueue() const { return m_Queue.Get(); }

        GfxSyncPoint CreateSyncPoint();
        void WaitOnGpu(const GfxSyncPoint& syncPoint);

    private:
        GfxCommandType m_Type;
        ComPtr<ID3D12CommandQueue> m_Queue;
        std::unique_ptr<GfxFence> m_Fence;

        std::vector<ComPtr<ID3D12CommandAllocator>> m_AllocatorStore; // 保存所有的 allocator 的引用
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_AllocatorFreeQueue;
    };

    class GfxCommandContext
    {
    public:
        GfxCommandContext(GfxDevice* device, GfxCommandQueue* queue);

        void Submit();

        void ResourceBarrier(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter);
        void ResourceBarrier(const CD3DX12_RESOURCE_BARRIER& barrier);
        void FlushResourceBarriers();

        GfxDevice* GetDevice() const { return m_Device; }
        GfxCommandQueue* GetQueue() const { return m_Queue; }
        ID3D12GraphicsCommandList* GetList() const { return m_List.Get(); }

    private:
        void Begin(ID3D12CommandAllocator* commandAllocator);
        void End();

        GfxDevice* m_Device;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_List;
        GfxCommandQueue* m_Queue;

        std::vector<CD3DX12_RESOURCE_BARRIER> m_ResourceBarriers;
        std::vector<GfxSyncPoint> m_SyncPointsToWait;
    };
}
