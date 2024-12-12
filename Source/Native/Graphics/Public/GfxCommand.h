#pragma once

#include "GfxDescriptor.h"
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

    class GfxFence final
    {
    public:
        GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue = 0);
        ~GfxFence();

        uint64_t GetCompletedValue() const;
        bool IsCompleted(uint64_t value) const;

        void WaitOnCpu(uint64_t value) const;
        void WaitOnGpu(ID3D12CommandQueue* queue, uint64_t value) const;

        uint64_t SignalNextValueOnCpu();
        uint64_t SignalNextValueOnGpu(ID3D12CommandQueue* queue);

        uint64_t GetNextValue() const { return m_Value + 1; }
        ID3D12Fence* GetFence() const { return m_Fence.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        HANDLE m_EventHandle;
        uint64_t m_Value; // 上一次 signal 的值，可能从 cpu 侧 signal，也可能从 gpu 侧 signal
    };

    class GfxSyncPoint final
    {
        friend class GfxCommandQueue;

    public:
        GfxSyncPoint(GfxFence* fence, uint64_t value) : m_Fence(fence), m_Value(value) {}

        void WaitOnCpu() const { m_Fence->WaitOnCpu(m_Value); }
        bool IsCompleted() const { return m_Fence->IsCompleted(m_Value); }

    private:
        GfxFence* m_Fence;
        const uint64_t m_Value;
    };

    struct GfxCommandQueueDesc
    {
        D3D12_COMMAND_LIST_TYPE Type;
        int32_t Priority;
        bool DisableGpuTimeout;
    };

    class GfxCommandQueue final
    {
    public:
        GfxCommandQueue(GfxDevice* device, const std::string& name, const GfxCommandQueueDesc& desc);

        GfxDevice* GetDevice() const { return m_Device; }
        D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
        ID3D12CommandQueue* GetQueue() const { return m_Queue.Get(); }

        GfxSyncPoint CreateSyncPoint();
        void WaitOnGpu(const GfxSyncPoint& syncPoint);

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> RequestCommandAllocator();
        GfxSyncPoint ReleaseCommandAllocator(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);

    private:
        GfxDevice* m_Device;
        D3D12_COMMAND_LIST_TYPE m_Type;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_Queue;
        std::unique_ptr<GfxFence> m_Fence;

        std::queue<std::pair<uint64_t, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>> m_CommandAllocators;
    };

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    enum class GfxCommandType
    {
        Direct, // 3D rendering engine
        AsyncCompute,
        AsyncCopy,

        // https://therealmjp.github.io/posts/gpu-memory-pool/
        // AsyncCopyHighPriority, // 高优先级的拷贝操作，或许有一天会用到
    };

    class GfxCommandContext;

    class GfxCommandManager final
    {
    public:
        GfxCommandManager(GfxDevice* device);

        GfxCommandQueue* GetQueue(GfxCommandType type) const;

        GfxCommandContext* RequestAndOpenContext(GfxCommandType type);
        void RecycleContext(GfxCommandContext* context);

        uint64_t GetCompletedFrameFence(bool useCache);
        bool IsFrameFenceCompleted(uint64_t fence, bool useCache);
        uint64_t GetNextFrameFence() const;

        void OnFrameEnd();
        void WaitForGpuIdle();

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        struct
        {
            std::unique_ptr<GfxCommandQueue> Queue;
            std::unique_ptr<GfxFence> FrameFence;
            std::queue<GfxCommandContext*> FreeContexts;
        } m_Commands[3]; // Matches GfxCommandType

        GfxDevice* m_Device;
        std::vector<std::unique_ptr<GfxCommandContext>> m_ContextStore; // 保存所有分配的 command context，用于释放资源
        uint64_t m_CompletedFence; // cache

        uint64_t SignalNextFence();
    };

    class GfxResource;

    class GfxCommandContext final
    {
    public:
        GfxCommandContext(GfxDevice* device, GfxCommandType type);

        void Open();
        GfxSyncPoint SubmitAndRelease();

        void TransitionResource(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter);
        void FlushResourceBarriers();

        void WaitOnGpu(const GfxSyncPoint& syncPoint);

        GfxDevice* GetDevice() const { return m_Device; }
        GfxCommandType GetType() const { return m_Type; }
        ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.Get(); }

    private:
        GfxDevice* m_Device;
        GfxCommandType m_Type;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

        std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;
        std::vector<GfxSyncPoint> m_SyncPointsToWait;

        GfxOfflineDescriptorCache<64> m_SrvCache;
        GfxOfflineDescriptorCache<32> m_UavCache;
        GfxOfflineDescriptorCache<16> m_SamplerCache;
    };
}
