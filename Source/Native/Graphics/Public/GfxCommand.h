#pragma once

#include "GfxDescriptor.h"
#include "Shader.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>

namespace march
{
    class GfxDevice;
    class GfxTexture;
    class GfxBuffer;

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

        uint64_t GetNextValue() const { return m_NextValue; }
        ID3D12Fence* GetFence() const { return m_Fence.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        HANDLE m_EventHandle;
        uint64_t m_NextValue; // 下一次 signal 的值，可能从 cpu 侧 signal，也可能从 gpu 侧 signal
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

        NumTypes
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
        } m_Commands[static_cast<size_t>(GfxCommandType::NumTypes)];

        GfxDevice* m_Device;
        std::vector<std::unique_ptr<GfxCommandContext>> m_ContextStore; // 保存所有分配的 command context，用于释放资源
        uint64_t m_CompletedFence; // cache

        uint64_t SignalNextFence();
    };

    class GfxResource;

    // 不要跨帧使用
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

        // 如果 root signature 变化，全部清空
        // 如果 root signature 没变，只有 dirty 时才重新设置 root descriptor table
        // 设置 root descriptor table 后，需要清除 dirty 标记
        // 切换 heap 时，需要强制设置为 dirty，重新设置 root descriptor table

        GfxOfflineDescriptorTable<64> m_GraphicsSrvUavCache[ShaderProgram::NumTypes];
        GfxOfflineDescriptorTable<16> m_GraphicsSamplerCache[ShaderProgram::NumTypes];
        std::unordered_map<GfxResource*, D3D12_RESOURCE_STATES> m_ViewResourceRequiredStates; // 暂存 Srv/Uav 资源需要的状态
        GfxDescriptorHeap* m_ViewHeap;
        GfxDescriptorHeap* m_SamplerHeap;
        bool m_IsDescriptorHeapChanged;

        std::unordered_map<int32_t, GfxTexture*> m_GlobalTextures;
        std::unordered_map<int32_t, GfxBuffer*> m_GlobalBuffers;

        void SetGraphicsSrv(ShaderProgramType type, uint32_t index, GfxResource* resource, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor);
        void SetGraphicsUav(ShaderProgramType type, uint32_t index, GfxResource* resource, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor);
        void SetGraphicsSampler(ShaderProgramType type, uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor);
        void SetGraphicsRootSrvUavTable(ShaderProgramType type, uint32_t index);
        void SetGraphicsRootSamplerTable(ShaderProgramType type, uint32_t index);
        void SetDescriptorHeaps();
    };
}
