#pragma once

#include "GfxDescriptor.h"
#include "GfxPipelineState.h"
#include "Shader.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <bitset>

namespace march
{
    class GfxDevice;
    class GfxResource;
    class GfxTexture;
    class GfxRenderTexture;
    class GfxBuffer;
    class Material;

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

    template <size_t Capacity>
    class GfxRootSrvCbvBufferCache
    {
    public:
        GfxRootSrvCbvBufferCache() = default;

        void Reset()
        {
            m_Num = 0;
            memset(m_Addresses, 0, sizeof(m_Addresses));
            m_IsConstantBuffer.reset();
            m_IsDirty.reset();
        }

        void Set(size_t index, D3D12_GPU_VIRTUAL_ADDRESS address, bool isConstantBuffer)
        {
            if (index < m_Num && m_Addresses[index] == address && m_IsConstantBuffer.test(index) == isConstantBuffer)
            {
                return;
            }

            m_Num = std::max(m_Num, index + 1);
            m_Addresses[index] = address;
            m_IsConstantBuffer.set(index, isConstantBuffer);
            m_IsDirty.set(index);
        }

        D3D12_GPU_VIRTUAL_ADDRESS Get(size_t index, bool* pOutIsConstantBuffer) const
        {
            assert(index < m_Num);
            *pOutIsConstantBuffer = m_IsConstantBuffer.test(index);
            return m_Addresses[index];
        }

        // 清空 dirty 标记
        void Apply() { m_IsDirty.reset(); }

        size_t GetNum() const { return m_Num; }
        bool IsEmpty() const { return m_Num == 0; }
        constexpr size_t GetCapacity() const { return Capacity; }
        bool IsDirty(size_t index) const { return m_IsDirty.test(index); }

    private:
        size_t m_Num; // 设置的最大 index + 1
        D3D12_GPU_VIRTUAL_ADDRESS m_Addresses[Capacity];
        std::bitset<Capacity> m_IsConstantBuffer;
        std::bitset<Capacity> m_IsDirty;
    };

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

        // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html
        // The maximum size of a root arguments is 64 DWORDs.
        // Descriptor tables cost 1 DWORD each.
        // Root constants cost 1 DWORD * NumConstants, by definition since they are collections of 32-bit values.
        // Raw/Structured Buffer SRVs/UAVs and CBVs cost 2 DWORDs.

        // 固定有 2 * shader-stage 个 descriptor table，所以 root cbv/srv 上限 (64 - 2 * 8) / 2 = 24 个
        // 最多可以有 24 个 root srv/cbv (在创建 root signature 时，root srv/cbv 是排在 descriptor table 前面的)
        GfxRootSrvCbvBufferCache<24> m_GraphicsSrvCbvBufferCache[ShaderProgram::NumTypes]; // root srv/cbv buffer

        // 如果 root signature 变化，root parameter cache 全部清空
        // 如果 root signature 没变，只有 dirty 时才重新设置 root descriptor table
        // 设置 root descriptor table 后，需要清除 dirty 标记
        // 切换 heap 时，需要强制设置为 dirty，重新设置所有 root descriptor table

        GfxOfflineDescriptorTable<64> m_GraphicsSrvUavCache[ShaderProgram::NumTypes];
        GfxOfflineDescriptorTable<16> m_GraphicsSamplerCache[ShaderProgram::NumTypes];
        std::unordered_map<GfxResource*, D3D12_RESOURCE_STATES> m_GraphicsViewResourceRequiredStates; // 暂存 srv/uav/cbv 资源需要的状态
        GfxDescriptorHeap* m_ViewHeap;
        GfxDescriptorHeap* m_SamplerHeap;

        std::vector<GfxRenderTexture*> m_ColorTargets;
        GfxRenderTexture* m_DepthStencilTarget;
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;

        GfxOutputDesc m_OutputDesc;

        ID3D12PipelineState* m_CurrentPipelineState;
        ID3D12RootSignature* m_CurrentGraphicsRootSignature;
        D3D12_PRIMITIVE_TOPOLOGY m_CurrentPrimitiveTopology;
        uint8_t m_CurrentStencilRef;
        bool m_IsStencilRefSet;

        std::unordered_map<int32_t, GfxTexture*> m_GlobalTextures;
        std::unordered_map<int32_t, GfxBuffer*> m_GlobalBuffers;

        GfxTexture* FindTexture(int32_t id, Material* material);
        GfxBuffer* FindBuffer(int32_t id, Material* material, bool isConstantBuffer);

        void SetGraphicsSrvCbvBuffer(ShaderProgramType type, uint32_t index, GfxResource* resource, D3D12_GPU_VIRTUAL_ADDRESS address, bool isConstantBuffer);
        void SetGraphicsSrv(ShaderProgramType type, uint32_t index, GfxResource* resource, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor);
        void SetGraphicsUav(ShaderProgramType type, uint32_t index, GfxResource* resource, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor);
        void SetGraphicsSampler(ShaderProgramType type, uint32_t index, D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor);
        void SetGraphicsRootSignatureAndParameters(GfxRootSignature* rootSignature, Material* material);
        void SetGraphicsRootDescriptorTablesAndHeaps(GfxRootSignature* rootSignature);
        void SetGraphicsRootSrvCbvBuffers();
        void TransitionGraphicsViewResources();
        void SetDescriptorHeaps();
    };
}
