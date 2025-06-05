#pragma once

#include "Engine/Memory/RefCounting.h"
#include "Engine/Rendering/D3D12Impl/GfxDescriptor.h"
#include "Engine/Rendering/D3D12Impl/GfxPipeline.h"
#include "Engine/Rendering/D3D12Impl/GfxBuffer.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Rendering/D3D12Impl/ShaderGraphics.h"
#include "Engine/Rendering/D3D12Impl/ShaderCompute.h"
#include "Engine/Rendering/D3D12Impl/MeshRenderer.h"
#include <directx/d3dx12.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdint.h>
#include <memory>
#include <unordered_map>
#include <optional>
#include <variant>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <atomic>

namespace march
{
    class GfxDevice;
    class GfxResource;
    class Material;
    class MeshRenderer;
    class GfxMesh;
    struct GfxSubMeshDesc;
    enum class GfxMeshGeometry;
    class GfxCommandManager;
    class GfxCommandContext;

    // 线程安全
    class GfxFence final
    {
    public:
        GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue = 0);
        ~GfxFence();

        uint64_t GetCompletedValue() const;
        bool IsCompleted(uint64_t value) const;

        void WaitOnCpu(uint64_t value);
        void WaitOnGpu(ID3D12CommandQueue* queue, uint64_t value) const;

        uint64_t SignalNextValueOnCpu();
        uint64_t SignalNextValueOnGpu(ID3D12CommandQueue* queue);
        uint64_t GetNextValue() const;

        ID3D12Fence* GetFence() const { return m_Fence.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        std::mutex m_EventMutex;
        HANDLE m_EventHandle;
        std::atomic_uint64_t m_NextValue; // 下一次 signal 的值，可能从 cpu 侧 signal，也可能从 gpu 侧 signal
    };

    class GfxSyncPoint final
    {
        friend class GfxCommandQueue;

        GfxFence* m_Fence;
        uint64_t m_Value;

    public:
        GfxSyncPoint(GfxFence* fence, uint64_t value) : m_Fence(fence), m_Value(value) {}
        GfxSyncPoint() : GfxSyncPoint(nullptr, 0) {}

        void WaitOnCpu() const { m_Fence->WaitOnCpu(m_Value); }
        bool IsCompleted() const { return m_Fence->IsCompleted(m_Value); }
        bool IsValid() const { return m_Fence != nullptr; }

        GfxSyncPoint(const GfxSyncPoint&) = default;
        GfxSyncPoint& operator=(const GfxSyncPoint&) = default;

        GfxSyncPoint(GfxSyncPoint&&) = default;
        GfxSyncPoint& operator=(GfxSyncPoint&&) = default;
    };

    struct GfxCommandQueueDesc
    {
        D3D12_COMMAND_LIST_TYPE Type;
        int32_t Priority;
        bool DisableGpuTimeout;
    };

    // 线程安全
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

        std::mutex m_CommandAllocatorMutex;
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

    namespace GfxCommands
    {
        struct BeginEvent
        {
            std::string Name;
        };

        struct EndEvent
        {
        };

        struct FlushResourceBarriers
        {
            size_t Offset;
            uint32_t Num;
        };

        struct SetRenderTargets
        {
            size_t ColorTargetOffset;
            uint32_t ColorTargetCount;
            std::optional<D3D12_CPU_DESCRIPTOR_HANDLE> DepthStencilTarget;
        };

        struct ClearColorTarget
        {
            D3D12_CPU_DESCRIPTOR_HANDLE Target;
            float Color[4];
        };

        struct ClearDepthStencilTarget
        {
            D3D12_CPU_DESCRIPTOR_HANDLE Target;
            D3D12_CLEAR_FLAGS Flags;
            float Depth;
            uint8_t Stencil;
        };

        struct SetViewports
        {
            size_t Offset;
            uint32_t Num;
        };

        struct SetScissorRects
        {
            size_t Offset;
            uint32_t Num;
        };

        struct SetPredication
        {
            ID3D12Resource* Buffer;
            uint32_t AlignedOffset;
            D3D12_PREDICATION_OP Operation;
        };

        struct SetPipelineState
        {
            ID3D12PipelineState* State;
        };

        struct SetStencilRef
        {
            uint8_t StencilRef;
        };

        struct SetPrimitiveTopology
        {
            D3D12_PRIMITIVE_TOPOLOGY Topology;
        };

        struct SetVertexBuffers
        {
            uint32_t StartSlot;
            size_t Offset;
            uint32_t Num;
        };

        struct SetIndexBuffer
        {
            std::optional<D3D12_INDEX_BUFFER_VIEW> View;
        };

        struct DrawIndexedInstanced
        {
            uint32_t IndexCountPerInstance;
            uint32_t InstanceCount;
            uint32_t StartIndexLocation;
            int32_t BaseVertexLocation;
            uint32_t StartInstanceLocation;
        };

        struct Dispatch
        {
            uint32_t ThreadGroupCountX;
            uint32_t ThreadGroupCountY;
            uint32_t ThreadGroupCountZ;
        };

        struct ResolveSubresource
        {
            ID3D12Resource* DstResource;
            uint32_t DstSubresource;
            ID3D12Resource* SrcResource;
            uint32_t SrcSubresource;
            DXGI_FORMAT Format;
        };

        struct UpdateSubresources
        {
            ID3D12Resource* Destination;
            ID3D12Resource* Intermediate;
            uint32_t IntermediateOffset;
            uint32_t FirstSubresource;
            uint32_t NumSubresources;
            size_t SrcDataOffset;
        };

        struct CopyBufferRegion
        {
            ID3D12Resource* DstBuffer;
            uint32_t DstOffset;
            ID3D12Resource* SrcBuffer;
            uint32_t SrcOffset;
            uint32_t NumBytes;
        };

        struct CopyTextureRegion
        {
            D3D12_TEXTURE_COPY_LOCATION Dst;
            D3D12_TEXTURE_COPY_LOCATION Src;
        };

        template <GfxPipelineType _PipelineType>
        struct SetRootSignature
        {
            ID3D12RootSignature* Signature;
        };

        template <GfxPipelineType _PipelineType>
        struct SetRootSrvCbvBuffer
        {
            uint32_t RootParamIndex;
            D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
            bool IsConstantBuffer; // 如果 true 则是 CBV 否则是 SRV
        };

        template <GfxPipelineType _PipelineType>
        struct SetRootDescriptorTables
        {
            size_t OfflineSrvUavDescriptorOffset;
            size_t OfflineSrvUavTableDataOffset;

            size_t OfflineSamplerDescriptorOffset;
            size_t OfflineSamplerTableDataOffset;
        };
    }

    struct GfxRootDescriptorTableDesc
    {
        uint32_t RootParamIndex; // 如果 NumDescriptors 是 0，该值无效
        const D3D12_CPU_DESCRIPTOR_HANDLE* OfflineDescriptors;
        uint32_t NumDescriptors;
        bool IsDirty;
    };

    struct GfxFutureSyncPoint final
    {
        size_t Index;
        uint64_t Version;
    };

    class GfxCommandList final
    {
        friend GfxCommandManager;

        using CommandType = std::variant<
            GfxCommands::BeginEvent,
            GfxCommands::EndEvent,
            GfxCommands::FlushResourceBarriers,
            GfxCommands::SetRenderTargets,
            GfxCommands::ClearColorTarget,
            GfxCommands::ClearDepthStencilTarget,
            GfxCommands::SetViewports,
            GfxCommands::SetScissorRects,
            GfxCommands::SetPredication,
            GfxCommands::SetPipelineState,
            GfxCommands::SetStencilRef,
            GfxCommands::SetPrimitiveTopology,
            GfxCommands::SetVertexBuffers,
            GfxCommands::SetIndexBuffer,
            GfxCommands::DrawIndexedInstanced,
            GfxCommands::Dispatch,
            GfxCommands::ResolveSubresource,
            GfxCommands::UpdateSubresources,
            GfxCommands::CopyBufferRegion,
            GfxCommands::CopyTextureRegion,
            GfxCommands::SetRootSignature<GfxPipelineType::Graphics>,
            GfxCommands::SetRootSignature<GfxPipelineType::Compute>,
            GfxCommands::SetRootSrvCbvBuffer<GfxPipelineType::Graphics>,
            GfxCommands::SetRootSrvCbvBuffer<GfxPipelineType::Compute>,
            GfxCommands::SetRootDescriptorTables<GfxPipelineType::Graphics>,
            GfxCommands::SetRootDescriptorTables<GfxPipelineType::Compute>
        >;

        std::vector<CommandType> m_Commands;

        std::vector<GfxSyncPoint> m_SyncPointsToWait;
        std::vector<GfxFutureSyncPoint> m_FutureSyncPointsToWait;

        std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;
        size_t m_ResourceBarrierFlushOffset;

        // 统一保存一些 Command 的参数，避免频繁创建销毁 vector
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_ColorTargets;
        std::vector<D3D12_VIEWPORT> m_Viewports;
        std::vector<D3D12_RECT> m_ScissorRects;
        std::vector<D3D12_VERTEX_BUFFER_VIEW> m_VertexBufferViews;
        std::vector<D3D12_SUBRESOURCE_DATA> m_SubresourceData;

        struct OfflineDescriptorTableData
        {
            uint32_t RootParamIndex; // 如果 NumDescriptors 是 0，该值无效
            uint32_t NumDescriptors;
            bool IsDirty;
        };

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_OfflineDescriptors;
        std::vector<OfflineDescriptorTableData> m_OfflineDescriptorTableData;

        GfxDescriptorHeap* m_ViewHeap;
        GfxDescriptorHeap* m_SamplerHeap;

        GfxCommandType const m_Type;
        GfxCommandQueue* const m_Queue;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_List;
        void* m_NsightAftermathHandle;

    public:
        GfxCommandList(GfxCommandType type, GfxCommandQueue* queue);
        ~GfxCommandList();

        void WaitOnGpu(const GfxSyncPoint& syncPoint);
        void WaitOnGpu(const GfxFutureSyncPoint& syncPoint);

        void BeginEvent(const std::string& name);
        void BeginEvent(std::string&& name);
        void EndEvent();

        void AddResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);
        void FlushResourceBarriers();

        void SetRenderTargets(uint32_t numColorTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* colorTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilTarget);
        void ClearColorTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, const float color[4]);
        void ClearDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, D3D12_CLEAR_FLAGS flags, float depth, uint8_t stencil);
        void SetViewports(uint32_t num, const D3D12_VIEWPORT* viewports);
        void SetScissorRects(uint32_t num, const D3D12_RECT* rects);
        void SetPredication(ID3D12Resource* buffer, uint32_t alignedOffset, D3D12_PREDICATION_OP operation);
        void SetPipelineState(ID3D12PipelineState* state);
        void SetStencilRef(uint8_t value);
        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology);
        void SetVertexBuffers(uint32_t startSlot, uint32_t numViews, const D3D12_VERTEX_BUFFER_VIEW* views);
        void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* view);
        void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation);
        void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);

        void ResolveSubresource(ID3D12Resource* dstResource, uint32_t dstSubresource, ID3D12Resource* srcResource, uint32_t srcSubresource, DXGI_FORMAT format);
        void UpdateSubresources(ID3D12Resource* destination, ID3D12Resource* intermediate, uint32_t intermediateOffset, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData);
        void CopyBufferRegion(ID3D12Resource* dstBuffer, uint32_t dstOffset, ID3D12Resource* srcBuffer, uint32_t srcOffset, uint32_t numBytes);
        void CopyTextureRegion(const D3D12_TEXTURE_COPY_LOCATION& dst, const D3D12_TEXTURE_COPY_LOCATION& src);

        template <GfxPipelineType _PipelineType>
        void SetRootSignature(ID3D12RootSignature* signature);

        template <GfxPipelineType _PipelineType>
        void SetRootSrvCbvBuffer(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, bool isConstantBuffer);

        template <GfxPipelineType _PipelineType>
        void SetRootDescriptorTables(
            const GfxRootDescriptorTableDesc(&srvUav)[GfxPipelineTraits<_PipelineType>::NumProgramTypes],
            const GfxRootDescriptorTableDesc(&samplers)[GfxPipelineTraits<_PipelineType>::NumProgramTypes]);

        GfxDevice* GetDevice() const { return m_Queue->GetDevice(); }
        GfxCommandQueue* GetQueue() const { return m_Queue; }
        GfxCommandType GetType() const { return m_Type; }
        bool HasFutureSyncPoints() const { return !m_FutureSyncPointsToWait.empty(); }

    private:
        GfxSyncPoint Execute(bool isImmediateMode);

        template <typename Func>
        void ResolveFutureSyncPoints(Func fn);

        void TranslateCommand(const GfxCommands::BeginEvent& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::EndEvent& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::FlushResourceBarriers& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetRenderTargets& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::ClearColorTarget& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::ClearDepthStencilTarget& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetViewports& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetScissorRects& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetPredication& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetPipelineState& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetStencilRef& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetPrimitiveTopology& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetVertexBuffers& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::SetIndexBuffer& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::DrawIndexedInstanced& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::Dispatch& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::ResolveSubresource& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::UpdateSubresources& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::CopyBufferRegion& cmd, bool isImmediateMode);
        void TranslateCommand(const GfxCommands::CopyTextureRegion& cmd, bool isImmediateMode);

        template <GfxPipelineType _PipelineType>
        void TranslateCommand(const GfxCommands::SetRootSignature<_PipelineType>& cmd, bool isImmediateMode);

        template <GfxPipelineType _PipelineType>
        void TranslateCommand(const GfxCommands::SetRootSrvCbvBuffer<_PipelineType>& cmd, bool isImmediateMode);

        template <GfxPipelineType _PipelineType>
        void TranslateCommand(const GfxCommands::SetRootDescriptorTables<_PipelineType>& cmd, bool isImmediateMode);
    };

    class GfxCommandManager final
    {
    public:
        GfxCommandManager(GfxDevice* device);
        ~GfxCommandManager();

        GfxCommandQueue* GetQueue(GfxCommandType type) const;

        GfxCommandContext* RequestContext(GfxCommandType type);
        void RecycleContext(GfxCommandContext* context);

        uint64_t GetCompletedFrameFence() const;
        bool IsFrameFenceCompleted(uint64_t fence) const;
        uint64_t GetNextFrameFence() const;
        void SignalNextFrameFence();
        void RefreshCompletedFrameFence(bool waitForLastFrame);

        GfxFutureSyncPoint Execute(GfxCommandList* list);
        GfxSyncPoint ExecuteImmediate(GfxCommandList* list);
        bool SyncOnMainThread();

        void WaitOnGpu(const GfxSyncPoint& syncPoint);
        void WaitOnGpu(const GfxFutureSyncPoint& syncPoint);

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        struct QueueData
        {
            std::unique_ptr<GfxCommandQueue> Queue;
            std::unique_ptr<GfxFence> FrameFence;
            std::queue<GfxCommandList*> FreeCmdLists;
        };

        QueueData m_QueueData[static_cast<size_t>(GfxCommandType::NumTypes)];

        GfxDevice* m_Device;
        std::vector<std::unique_ptr<GfxCommandContext>> m_ContextStore; // 保存所有分配的 command context，用于释放资源
        std::vector<std::unique_ptr<GfxCommandList>> m_CmdListStore;    // 保存所有分配的 command list，用于释放资源
        std::queue<GfxCommandContext*> m_FreeContexts;
        std::atomic_uint64_t m_CompletedFrameFence; // cache

        std::thread m_RHIThread;
        std::mutex m_RHIMutex;
        std::condition_variable m_MainThreadCVar;
        std::condition_variable m_RHIThreadCVar;
        bool m_IsSwappingCmdListBuffers;

        using CommandType = std::variant<
            GfxCommandList*,
            GfxSyncPoint,
            GfxFutureSyncPoint
        >;

        // 双缓冲保存 Main Thread 和 RHI Thread 的 Command，每帧末尾交换
        std::vector<CommandType> m_CmdBuffers[2];
        size_t m_MainThreadCmdBufferIndex;
        size_t m_RHIThreadCmdBufferIndex;
        uint64_t m_CmdBufferVersion;

        bool m_IsRHIThreadRunning;
        std::atomic_bool m_IsRHIThreadExecuted;

        void RecycleCommandList(GfxCommandList* list);
        void RHIThreadProc();
        bool SyncOnRHIThread();
    };

    enum class GfxClearFlags
    {
        None = 0,
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        DepthStencil = Depth | Stencil,
        All = Color | Depth | Stencil,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GfxClearFlags);

    struct GfxRenderTargetDesc
    {
        GfxTexture* Texture;
        GfxCubemapFace Face;
        uint32_t WOrArraySlice;
        uint32_t MipSlice;

        GfxRenderTargetDesc() = default;
        GfxRenderTargetDesc(GfxTexture* texture);

        static GfxRenderTargetDesc Tex2D(GfxTexture* texture, uint32_t mipSlice = 0);
        static GfxRenderTargetDesc Tex3D(GfxTexture* texture, uint32_t wSlice = 0, uint32_t mipSlice = 0);
        static GfxRenderTargetDesc Cube(GfxTexture* texture, GfxCubemapFace face, uint32_t mipSlice = 0);
        static GfxRenderTargetDesc Tex2DArray(GfxTexture* texture, uint32_t arraySlice = 0, uint32_t mipSlice = 0);
        static GfxRenderTargetDesc CubeArray(GfxTexture* texture, GfxCubemapFace face, uint32_t arraySlice = 0, uint32_t mipSlice = 0);
    };

    template <GfxPipelineType _PipelineType>
    class GfxPipelineParameterCache final
    {
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;

        // https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html
        // The maximum size of a root arguments is 64 DWORDs.
        // Descriptor tables cost 1 DWORD each.
        // Root constants cost 1 DWORD * NumConstants, by definition since they are collections of 32-bit values.
        // Raw/Structured Buffer SRVs/UAVs and CBVs cost 2 DWORDs.

        // Shader Stage 的数量
        static constexpr size_t NumProgramTypes = PipelineTraits::NumProgramTypes;

        // 每个 Shader Stage 固定有 srv/uav 和 sampler 两个 descriptor table
        static constexpr size_t NumDescriptorTables = 2 * NumProgramTypes;

        // 剩下的空间都分给 root srv/cbv buffer
        // 在创建 root signature 时，root srv/cbv buffer 是排在 descriptor table 前面的
        static constexpr size_t NumMaxRootSrvCbvBuffers = (64 - NumDescriptorTables) / 2;

        using RootSignatureType = ShaderRootSignature<NumProgramTypes>;

        GfxDevice* m_Device;
        RootSignatureType* m_RootSignature;
        bool m_IsRootSignatureDirty;

        // 如果 root signature 变化，root parameter cache 全部清空
        // 如果 root signature 没变，只有 dirty 时才重新设置 root descriptor table
        // 设置 root descriptor table 后，需要清除 dirty 标记
        // 切换 heap 时，需要强制设置为 dirty，重新设置所有 root descriptor table

        GfxRootSrvCbvBufferCache<NumMaxRootSrvCbvBuffers> m_SrvCbvBufferCache[NumProgramTypes];
        GfxOfflineDescriptorTable<64> m_SrvUavCache[NumProgramTypes];
        GfxOfflineDescriptorTable<16> m_SamplerCache[NumProgramTypes];

        struct ResourceStateKey
        {
            RefCountPtr<GfxResource> Resource;
            uint32_t SubresourceIndex; // -1 表示整个资源

            bool operator ==(const ResourceStateKey& other) const
            {
                return Resource == other.Resource && SubresourceIndex == other.SubresourceIndex;
            }
        };

        struct ResourceStateKeyHash
        {
            size_t operator()(const ResourceStateKey& key) const
            {
                return std::hash<RefCountPtr<GfxResource>>()(key.Resource) ^ std::hash<uint32_t>()(key.SubresourceIndex);
            }
        };

        // 暂存 srv/uav/cbv 资源需要的状态
        std::unordered_map<ResourceStateKey, D3D12_RESOURCE_STATES, ResourceStateKeyHash> m_StagedResourceStates;

        static constexpr bool AllowPixelProgram = PipelineTraits::PixelProgramType < NumProgramTypes;
        static constexpr bool IsPixelProgram(size_t type) { return type == PipelineTraits::PixelProgramType; }

        void StageResourceState(RefCountPtr<GfxResource> resource, D3D12_RESOURCE_STATES state)
        {
            uint32_t subresourceIndex = -1;
            m_StagedResourceStates[ResourceStateKey{ resource, subresourceIndex }] |= state;
        }

        void StageTextureMipSliceSubresourceState(GfxTexture* texture, GfxTextureElement element, uint32_t mipSlice, D3D12_RESOURCE_STATES state)
        {
            GfxTextureDimension dimension = texture->GetDesc().Dimension;
            RefCountPtr<GfxResource> resource = texture->GetUnderlyingResource();

            for (uint32_t arraySlice = 0; arraySlice < texture->GetDesc().DepthOrArraySize; arraySlice++)
            {
                if (dimension == GfxTextureDimension::Cube || dimension == GfxTextureDimension::CubeArray)
                {
                    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
                    {
                        GfxCubemapFace face = static_cast<GfxCubemapFace>(faceIndex);
                        uint32_t subresourceIndex = texture->GetSubresourceIndex(element, face, arraySlice, mipSlice);
                        m_StagedResourceStates[ResourceStateKey{ resource, subresourceIndex }] |= state;
                    }
                }
                else
                {
                    uint32_t subresourceIndex = texture->GetSubresourceIndex(element, arraySlice, mipSlice);
                    m_StagedResourceStates[ResourceStateKey{ resource, subresourceIndex }] |= state;
                }
            }
        }

        void SetSrvCbvBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element, bool isConstantBuffer)
        {
            D3D12_GPU_VIRTUAL_ADDRESS address = buffer->GetGpuVirtualAddress(element);
            m_SrvCbvBufferCache[type].Set(static_cast<size_t>(index), address, isConstantBuffer);

            D3D12_RESOURCE_STATES state;

            if (isConstantBuffer)
            {
                state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            }
            else if constexpr (AllowPixelProgram)
            {
                state = IsPixelProgram(type)
                    ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                    : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            else
            {
                state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }

            // 记录状态，之后会统一使用 ResourceBarrier
            StageResourceState(buffer->GetUnderlyingResource(), state);
        }

        void SetSrvTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element, std::optional<uint32_t> mipSlice)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSrv(element, mipSlice);
            m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

            D3D12_RESOURCE_STATES state;

            if constexpr (AllowPixelProgram)
            {
                state = IsPixelProgram(type)
                    ? D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
                    : D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }
            else
            {
                state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
            }

            // 记录状态，之后会统一使用 ResourceBarrier
            if (mipSlice)
            {
                StageTextureMipSliceSubresourceState(texture, element, *mipSlice, state);
            }
            else
            {
                StageResourceState(texture->GetUnderlyingResource(), state);
            }
        }

        void SetUavBuffer(size_t type, uint32_t index, GfxBuffer* buffer, GfxBufferElement element)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = buffer->GetUav(element);
            m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

            // 记录状态，之后会统一使用 ResourceBarrier
            StageResourceState(buffer->GetUnderlyingResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        void SetUavTexture(size_t type, uint32_t index, GfxTexture* texture, GfxTextureElement element, uint32_t mipSlice)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetUav(element, mipSlice);
            m_SrvUavCache[type].Set(static_cast<size_t>(index), offlineDescriptor);

            // 记录状态，之后会统一使用 ResourceBarrier
            StageTextureMipSliceSubresourceState(texture, element, mipSlice, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        void SetSampler(size_t type, uint32_t index, GfxTexture* texture)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE offlineDescriptor = texture->GetSampler();
            m_SamplerCache[type].Set(static_cast<size_t>(index), offlineDescriptor);
        }

        void SetRootSrvCbvBuffers(GfxCommandList* cmd)
        {
            for (auto& cache : m_SrvCbvBufferCache)
            {
                for (size_t i = 0; i < cache.GetNum(); i++)
                {
                    if (!cache.IsDirty(i))
                    {
                        continue;
                    }

                    bool isConstantBuffer = false;
                    D3D12_GPU_VIRTUAL_ADDRESS address = cache.Get(i, &isConstantBuffer);
                    cmd->SetRootSrvCbvBuffer<_PipelineType>(static_cast<uint32_t>(i), address, isConstantBuffer);
                }

                cache.Apply();
            }
        }

    public:
        GfxPipelineParameterCache(GfxDevice* device)
            : m_Device(device)
            , m_RootSignature(nullptr)
            , m_IsRootSignatureDirty(true)
            , m_SrvCbvBufferCache{}
            , m_SrvUavCache{}
            , m_SamplerCache{}
            , m_StagedResourceStates{}
        {
        }

        void Reset()
        {
            m_RootSignature = nullptr;
            m_IsRootSignatureDirty = false;
            for (auto& cache : m_SrvCbvBufferCache) cache.Reset();
            for (auto& cache : m_SrvUavCache) cache.Reset();
            for (auto& cache : m_SamplerCache) cache.Reset();
            m_StagedResourceStates.clear();
        }

        void SetRootSignature(RootSignatureType* rootSignature)
        {
            // 内部的 ID3D12RootSignature 是复用的，如果 ID3D12RootSignature 变了，说明根签名发生了结构上的变化
            if (m_RootSignature == nullptr || m_RootSignature->GetD3DRootSignature() != rootSignature->GetD3DRootSignature())
            {
                m_IsRootSignatureDirty = true;

                // 删掉旧的 view
                for (auto& cache : m_SrvCbvBufferCache) cache.Reset();
                for (auto& cache : m_SrvUavCache) cache.Reset();
                for (auto& cache : m_SamplerCache) cache.Reset();
                m_StagedResourceStates.clear();
            }

            m_RootSignature = rootSignature;
        }

        template <typename FindBufferFunc>
        void SetSrvCbvBuffers(FindBufferFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& buf : m_RootSignature->GetSrvCbvBuffers(i))
                {
                    GfxBufferElement element = GfxBufferElement::StructuredData;

                    if (GfxBuffer* buffer = fn(buf, &element))
                    {
                        SetSrvCbvBuffer(i, buf.RootParameterIndex, buffer, element, buf.IsConstantBuffer);
                    }
                    else if (buf.IsConstantBuffer)
                    {
                        throw GfxException(StringUtils::Format("Failed to find root cbv buffer parameter '{}'", ShaderUtils::GetStringFromId(buf.Id)));
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root srv buffer parameter '{}'", ShaderUtils::GetStringFromId(buf.Id)));
                    }
                }
            }
        }

        void UpdateSrvCbvBuffer(int32_t id, GfxBuffer* buffer, GfxBufferElement element)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& buf : m_RootSignature->GetSrvCbvBuffers(i))
                {
                    if (buf.Id != id)
                    {
                        continue;
                    }

                    SetSrvCbvBuffer(i, buf.RootParameterIndex, buffer, element, buf.IsConstantBuffer);
                }
            }
        }

        template <typename FindTextureFunc>
        void SetSrvTexturesAndSamplers(FindTextureFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& tex : m_RootSignature->GetSrvTextures(i))
                {
                    GfxTextureElement element = GfxTextureElement::Default;
                    std::optional<uint32_t> mipSlice = std::nullopt;

                    if (GfxTexture* texture = fn(tex, &element, &mipSlice))
                    {
                        SetSrvTexture(i, tex.DescriptorTableSlotTexture, texture, element, mipSlice);

                        if (tex.DescriptorTableSlotSampler.has_value())
                        {
                            SetSampler(i, *tex.DescriptorTableSlotSampler, texture);
                        }
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root srv texture parameter '{}'", ShaderUtils::GetStringFromId(tex.Id)));
                    }
                }
            }
        }

        template <typename FindBufferFunc>
        void SetUavBuffers(FindBufferFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& buf : m_RootSignature->GetUavBuffers(i))
                {
                    GfxBufferElement element = GfxBufferElement::StructuredData;

                    if (GfxBuffer* buffer = fn(buf, &element))
                    {
                        SetUavBuffer(i, buf.DescriptorTableSlot, buffer, element);
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root uav buffer parameter '{}'", ShaderUtils::GetStringFromId(buf.Id)));
                    }
                }
            }
        }

        template <typename FindTextureFunc>
        void SetUavTextures(FindTextureFunc fn)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                for (const auto& tex : m_RootSignature->GetUavTextures(i))
                {
                    GfxTextureElement element = GfxTextureElement::Default;
                    std::optional<uint32_t> mipSlice = std::nullopt;

                    if (GfxTexture* texture = fn(tex, &element, &mipSlice))
                    {
                        SetUavTexture(i, tex.DescriptorTableSlot, texture, element, mipSlice.value_or(0));
                    }
                    else
                    {
                        throw GfxException(StringUtils::Format("Failed to find root uav texture parameter '{}'", ShaderUtils::GetStringFromId(tex.Id)));
                    }
                }
            }
        }

        template <typename TransitionFunc>
        void TransitionResources(TransitionFunc fn)
        {
            for (const auto& [key, state] : m_StagedResourceStates)
            {
                fn(key.Resource, key.SubresourceIndex, state);
            }

            m_StagedResourceStates.clear();
        }

        void SetRootDescriptorTables(GfxCommandList* cmd);

        void Apply(GfxCommandList* cmd)
        {
            if (m_IsRootSignatureDirty)
            {
                cmd->SetRootSignature<_PipelineType>(m_RootSignature->GetD3DRootSignature());
                m_IsRootSignatureDirty = false;
            }

            SetRootSrvCbvBuffers(cmd);
            SetRootDescriptorTables(cmd);
        }
    };

    // 不要跨帧使用
    class GfxCommandContext final
    {
    public:
        GfxCommandContext(GfxDevice* device);

        void Open(GfxCommandList* commandList);
        GfxFutureSyncPoint SubmitAndRelease();
        GfxSyncPoint SubmitImmediateAndRelease();

        void BeginEvent(const std::string& name);
        void BeginEvent(std::string&& name);
        void EndEvent();

        void TransitionResource(RefCountPtr<GfxResource> resource, D3D12_RESOURCE_STATES stateAfter);
        void TransitionSubresource(RefCountPtr<GfxResource> resource, uint32_t subresource, D3D12_RESOURCE_STATES stateAfter);
        void FlushResourceBarriers();

        void WaitOnGpu(const GfxSyncPoint& syncPoint);
        void WaitOnGpu(const GfxFutureSyncPoint& syncPoint);

        void SetTexture(const std::string& name, GfxTexture* value, GfxTextureElement element = GfxTextureElement::Default, std::optional<uint32_t> mipSlice = std::nullopt);
        void SetTexture(int32_t id, GfxTexture* value, GfxTextureElement element = GfxTextureElement::Default, std::optional<uint32_t> mipSlice = std::nullopt);
        void UnsetTextures();
        void SetBuffer(const std::string& name, GfxBuffer* value, GfxBufferElement element = GfxBufferElement::StructuredData);
        void SetBuffer(int32_t id, GfxBuffer* value, GfxBufferElement element = GfxBufferElement::StructuredData);
        void UnsetBuffers();
        void UnsetTexturesAndBuffers();

        void SetColorTarget(const GfxRenderTargetDesc& colorTarget);
        void SetDepthStencilTarget(const GfxRenderTargetDesc& depthStencilTarget);
        void SetRenderTarget(const GfxRenderTargetDesc& colorTarget, const GfxRenderTargetDesc& depthStencilTarget);
        void SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets);
        void SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets, const GfxRenderTargetDesc& depthStencilTarget);
        void ClearRenderTargets(GfxClearFlags flags = GfxClearFlags::All, const float color[4] = DirectX::Colors::Black, float depth = GfxUtils::FarClipPlaneDepth, uint8_t stencil = 0);
        void ClearColorTarget(uint32_t index, const float color[4] = DirectX::Colors::Black);
        void ClearDepthStencilTarget(float depth = GfxUtils::FarClipPlaneDepth, uint8_t stencil = 0);
        void SetViewport(const D3D12_VIEWPORT& viewport);
        void SetViewports(uint32_t numViewports, const D3D12_VIEWPORT* viewports);
        void SetScissorRect(const D3D12_RECT& rect);
        void SetScissorRects(uint32_t numRects, const D3D12_RECT* rects);
        void SetDefaultViewport();
        void SetDefaultScissorRect();
        void SetDepthBias(int32_t bias, float slopeScaledBias, float clamp);
        void SetDefaultDepthBias();
        void SetWireframe(bool value);

        // Use this method to denote that subsequent rendering and resource manipulation commands are not actually performed
        // if the resulting predicate data of the predicate is equal to the operation specified.
        void SetPredication(GfxBuffer* buffer, uint32_t alignedOffset = 0, D3D12_PREDICATION_OP operation = D3D12_PREDICATION_OP_EQUAL_ZERO);

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex);
        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix);
        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex);
        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix);
        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex);
        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix);

        void DrawMeshRenderers(const MeshRendererBatch& batch, const std::string& lightMode);

        void DispatchCompute(ComputeShader* shader, const std::string& kernelName, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
        void DispatchCompute(ComputeShader* shader, size_t kernelIndex, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
        void DispatchComputeByThreadCount(ComputeShader* shader, const std::string& kernelName, uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ);
        void DispatchComputeByThreadCount(ComputeShader* shader, size_t kernelIndex, uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ);

        void ResolveTexture(GfxTexture* source, GfxTexture* destination);
        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement);
        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, uint32_t sourceOffsetInBytes, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement, uint32_t destinationOffsetInBytes, uint32_t sizeInBytes);
        void UpdateSubresources(RefCountPtr<GfxResource> destination, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData);
        void CopyTexture(GfxTexture* sourceTexture, GfxTextureElement sourceElement, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, GfxTextureElement destinationElement, uint32_t destinationArraySlice, uint32_t destinationMipSlice);
        void CopyTexture(GfxTexture* sourceTexture, GfxTextureElement sourceElement, GfxCubemapFace sourceFace, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, GfxTextureElement destinationElement, GfxCubemapFace destinationFace, uint32_t destinationArraySlice, uint32_t destinationMipSlice);
        void CopyTexture(GfxTexture* sourceTexture, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, uint32_t destinationArraySlice, uint32_t destinationMipSlice);
        void CopyTexture(GfxTexture* sourceTexture, GfxCubemapFace sourceFace, uint32_t sourceArraySlice, uint32_t sourceMipSlice, GfxTexture* destinationTexture, GfxCubemapFace destinationFace, uint32_t destinationArraySlice, uint32_t destinationMipSlice);

        void PrepareForPresent(GfxRenderTexture* texture);

        GfxDevice* GetDevice() const { return m_Device; }
        GfxCommandType GetType() const { return m_CommandList->GetType(); }

    private:
        GfxDevice* m_Device;
        GfxCommandList* m_CommandList;

        GfxPipelineParameterCache<GfxPipelineType::Graphics> m_GraphicsViewCache;
        GfxPipelineParameterCache<GfxPipelineType::Compute> m_ComputeViewCache;

        struct RenderTargetData
        {
            GfxTexture* Texture;
            D3D12_CPU_DESCRIPTOR_HANDLE RtvDsv;
        };

        RenderTargetData m_ColorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        RenderTargetData m_DepthStencilTarget;

        uint32_t m_NumViewports;
        D3D12_VIEWPORT m_Viewports[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
        uint32_t m_NumScissorRects;
        D3D12_RECT m_ScissorRects[D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];

        GfxOutputDesc m_OutputDesc;

        GfxBuffer* m_CurrentPredicationBuffer;
        uint32_t m_CurrentPredicationOffset;
        D3D12_PREDICATION_OP m_CurrentPredicationOperation;

        ID3D12PipelineState* m_CurrentPipelineState;

        D3D12_PRIMITIVE_TOPOLOGY m_CurrentPrimitiveTopology;
        D3D12_VERTEX_BUFFER_VIEW m_CurrentVertexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_CurrentIndexBuffer;
        std::optional<uint8_t> m_CurrentStencilRef;

        struct GlobalTextureData
        {
            GfxTexture* Texture;
            GfxTextureElement Element;
            std::optional<uint32_t> MipSlice;
        };

        struct GlobalBufferData
        {
            GfxBuffer* Buffer;
            GfxBufferElement Element;
        };

        std::unordered_map<int32_t, GlobalTextureData> m_GlobalTextures;
        std::unordered_map<int32_t, GlobalBufferData> m_GlobalBuffers;

        GfxBuffer m_InstanceBuffer;

        void Reset();

        void SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets, const GfxRenderTargetDesc* depthStencilTarget);
        static D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsvFromRenderTargetDesc(const GfxRenderTargetDesc& desc);

        GfxTexture* GetFirstRenderTarget() const;
        GfxTexture* FindTexture(int32_t id, GfxTextureElement* pOutElement, std::optional<uint32_t>* pOutMipSlice);
        GfxTexture* FindTexture(int32_t id, Material* material, GfxTextureElement* pOutElement, std::optional<uint32_t>* pOutMipSlice);
        GfxBuffer* FindComputeBuffer(int32_t id, bool isConstantBuffer, GfxBufferElement* pOutElement);
        GfxBuffer* FindGraphicsBuffer(int32_t id, bool isConstantBuffer, Material* material, size_t passIndex, GfxBufferElement* pOutElement);

        void SetInstanceBufferData(uint32_t numInstances, const MeshRendererBatch::InstanceData* instances);

        void SetGraphicsPipelineParameters(Material* material, size_t passIndex);
        void UpdateGraphicsPipelineInstanceDataParameter(uint32_t numInstances, const MeshRendererBatch::InstanceData* instances);
        void ApplyGraphicsPipelineParameters(ID3D12PipelineState* pso);
        void SetAndApplyComputePipelineParameters(ID3D12PipelineState* pso, ComputeShader* shader, size_t kernelIndex);

        void SetResolvedRenderState(const ShaderPassRenderState& state);
        void SetStencilRef(uint8_t value);

        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY value);
        void SetVertexBuffer(GfxBuffer* buffer);
        void SetIndexBuffer(GfxBuffer* buffer);
    };

    template <typename Func>
    void GfxCommandList::ResolveFutureSyncPoints(Func fn)
    {
        for (const GfxFutureSyncPoint& syncPoint : m_FutureSyncPointsToWait)
        {
            m_SyncPointsToWait.emplace_back(fn(syncPoint));
        }

        m_FutureSyncPointsToWait.clear();
    }

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::SetRootSignature(ID3D12RootSignature* signature)
    {
        using MyCommand = GfxCommands::SetRootSignature<_PipelineType>;

        m_Commands.emplace_back(MyCommand{ signature });
    }

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::SetRootSrvCbvBuffer(uint32_t rootParamIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation, bool isConstantBuffer)
    {
        using MyCommand = GfxCommands::SetRootSrvCbvBuffer<_PipelineType>;

        m_Commands.emplace_back(MyCommand{ rootParamIndex, bufferLocation, isConstantBuffer });
    }

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::SetRootDescriptorTables(
        const GfxRootDescriptorTableDesc(&srvUav)[GfxPipelineTraits<_PipelineType>::NumProgramTypes],
        const GfxRootDescriptorTableDesc(&samplers)[GfxPipelineTraits<_PipelineType>::NumProgramTypes])
    {
        using MyCommand = GfxCommands::SetRootDescriptorTables<_PipelineType>;
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;

        auto set = [this](
            const GfxRootDescriptorTableDesc(&desc)[PipelineTraits::NumProgramTypes],
            size_t& descriptorOffset,
            size_t& tableDataOffset)
        {
            descriptorOffset = m_OfflineDescriptors.size();
            tableDataOffset = m_OfflineDescriptorTableData.size();

            for (size_t i = 0; i < PipelineTraits::NumProgramTypes; i++)
            {
                OfflineDescriptorTableData& data = m_OfflineDescriptorTableData.emplace_back();
                data.RootParamIndex = desc[i].RootParamIndex;
                data.NumDescriptors = desc[i].NumDescriptors;
                data.IsDirty = desc[i].IsDirty;

                const D3D12_CPU_DESCRIPTOR_HANDLE* descriptors = desc[i].OfflineDescriptors;
                m_OfflineDescriptors.insert(m_OfflineDescriptors.end(), descriptors, descriptors + data.NumDescriptors);
            }
        };

        MyCommand cmd{};
        set(srvUav, cmd.OfflineSrvUavDescriptorOffset, cmd.OfflineSrvUavTableDataOffset);
        set(samplers, cmd.OfflineSamplerDescriptorOffset, cmd.OfflineSamplerTableDataOffset);
        m_Commands.emplace_back(cmd);
    }

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::TranslateCommand(const GfxCommands::SetRootSignature<_PipelineType>& cmd, bool isImmediateMode)
    {
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;

        ID3D12GraphicsCommandList* list = m_List.Get();
        (list->*PipelineTraits::SetRootSignature)(cmd.Signature);
    }

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::TranslateCommand(const GfxCommands::SetRootSrvCbvBuffer<_PipelineType>& cmd, bool isImmediateMode)
    {
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;

        ID3D12GraphicsCommandList* list = m_List.Get();
        UINT rootParamIndex = static_cast<UINT>(cmd.RootParamIndex);

        if (cmd.IsConstantBuffer)
        {
            (list->*PipelineTraits::SetRootConstantBufferView)(rootParamIndex, cmd.BufferLocation);
        }
        else
        {
            (list->*PipelineTraits::SetRootShaderResourceView)(rootParamIndex, cmd.BufferLocation);
        }
    }

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::TranslateCommand(const GfxCommands::SetRootDescriptorTables<_PipelineType>& cmd, bool isImmediateMode)
    {
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;

        if (isImmediateMode)
        {
            throw std::runtime_error("Immediate mode does not support SetRootDescriptorTables command.");
        }

        auto allocateTables = [this](
            GfxOnlineDescriptorMultiAllocator* allocator,
            size_t offlineDescriptorOffset,
            size_t offlineTableDataOffset,
            D3D12_GPU_DESCRIPTOR_HANDLE(&tables)[PipelineTraits::NumProgramTypes],
            uint32_t(&nums)[PipelineTraits::NumProgramTypes],
            GfxDescriptorHeap** ppOutHeap) -> bool
        {
            const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors[PipelineTraits::NumProgramTypes]{};

            for (size_t i = 0; i < PipelineTraits::NumProgramTypes; i++)
            {
                const auto& tableData = m_OfflineDescriptorTableData[offlineTableDataOffset + i];
                offlineDescriptors[i] = m_OfflineDescriptors.data() + offlineDescriptorOffset;
                offlineDescriptorOffset += tableData.NumDescriptors;
            }

            for (uint32_t numTry = 0; numTry < 2; numTry++)
            {
                uint32_t totalNumDescriptors = 0;

                // 从第 2 次尝试开始，因为发生了 rollover 导致 heap 变化，所以 table 要全部重新分配
                const bool forceAllocate = numTry > 0;

                for (size_t i = 0; i < PipelineTraits::NumProgramTypes; i++)
                {
                    const auto& tableData = m_OfflineDescriptorTableData[offlineTableDataOffset + i];
                    nums[i] = (tableData.IsDirty || forceAllocate) ? tableData.NumDescriptors : 0;
                    totalNumDescriptors += nums[i];
                }

                if (totalNumDescriptors == 0)
                {
                    return false;
                }

                if (allocator->AllocateMany(std::size(tables), offlineDescriptors, nums, tables, ppOutHeap))
                {
                    return true;
                }

                // 当前的 heap 不够，切换 heap
                allocator->Rollover();
            }

            return false;
        };

        auto setHeaps = [this](
            GfxDescriptorHeap* viewHeap,
            GfxDescriptorHeap* samplerHeap)
        {
            bool isChanged = false;

            if (viewHeap != nullptr && m_ViewHeap != viewHeap)
            {
                m_ViewHeap = viewHeap;
                isChanged = true;
            }

            if (samplerHeap != nullptr && m_SamplerHeap != samplerHeap)
            {
                m_SamplerHeap = samplerHeap;
                isChanged = true;
            }

            if (isChanged)
            {
                UINT numHeaps = 0;
                ID3D12DescriptorHeap* heaps[2]{};

                if (m_ViewHeap != nullptr)
                {
                    heaps[numHeaps++] = m_ViewHeap->GetD3DDescriptorHeap();
                }

                if (m_SamplerHeap != nullptr)
                {
                    heaps[numHeaps++] = m_SamplerHeap->GetD3DDescriptorHeap();
                }

                m_List->SetDescriptorHeaps(numHeaps, heaps);
            }
        };

        auto setTables = [this](
            size_t offlineTableDataOffset,
            const D3D12_GPU_DESCRIPTOR_HANDLE(&tables)[PipelineTraits::NumProgramTypes],
            const uint32_t(&nums)[PipelineTraits::NumProgramTypes])
        {
            ID3D12GraphicsCommandList* list = m_List.Get();

            for (size_t i = 0; i < PipelineTraits::NumProgramTypes; i++)
            {
                if (nums[i] == 0)
                {
                    continue;
                }

                const auto& tableData = m_OfflineDescriptorTableData[offlineTableDataOffset + i];
                (list->*PipelineTraits::SetRootDescriptorTable)(static_cast<UINT>(tableData.RootParamIndex), tables[i]);
            }
        };

        // ------------------------------------------------------------
        // SRV & UAV
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE srvUavTables[PipelineTraits::NumProgramTypes]{};
        uint32_t numSrvUav[PipelineTraits::NumProgramTypes]{};
        GfxDescriptorHeap* viewHeap = nullptr;

        bool hasSrvUav = allocateTables(
            m_Queue->GetDevice()->GetOnlineViewDescriptorAllocator(),
            cmd.OfflineSrvUavDescriptorOffset,
            cmd.OfflineSrvUavTableDataOffset,
            srvUavTables,
            numSrvUav,
            &viewHeap);

        // ------------------------------------------------------------
        // SAMPLER
        // ------------------------------------------------------------

        D3D12_GPU_DESCRIPTOR_HANDLE samplerTables[PipelineTraits::NumProgramTypes]{};
        uint32_t numSamplers[PipelineTraits::NumProgramTypes]{};
        GfxDescriptorHeap* samplerHeap = nullptr;

        bool hasSampler = allocateTables(
            m_Queue->GetDevice()->GetOnlineSamplerDescriptorAllocator(),
            cmd.OfflineSamplerDescriptorOffset,
            cmd.OfflineSamplerTableDataOffset,
            samplerTables,
            numSamplers,
            &samplerHeap);

        // ------------------------------------------------------------
        // Apply
        // ------------------------------------------------------------

        if (hasSrvUav || hasSampler)
        {
            setHeaps(viewHeap, samplerHeap);

            if (hasSrvUav)
            {
                setTables(cmd.OfflineSrvUavTableDataOffset, srvUavTables, numSrvUav);
            }

            if (hasSampler)
            {
                setTables(cmd.OfflineSamplerTableDataOffset, samplerTables, numSamplers);
            }
        }
    }

    template <GfxPipelineType _PipelineType>
    void GfxPipelineParameterCache<_PipelineType>::SetRootDescriptorTables(GfxCommandList* cmd)
    {
        auto set = [this](
            GfxRootDescriptorTableDesc(&desc)[NumProgramTypes],
            auto&& getRootParamIndexFunc,
            auto&& caches)
        {
            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                std::optional<uint32_t> rootParamIndex = (m_RootSignature->*getRootParamIndexFunc)(i);
                auto& cache = caches[i];

                if (rootParamIndex.has_value() && !cache.IsEmpty())
                {
                    desc[i].RootParamIndex = *rootParamIndex;
                    desc[i].OfflineDescriptors = cache.GetDescriptors();
                    desc[i].NumDescriptors = static_cast<uint32_t>(cache.GetNum());
                }
                else
                {
                    desc[i].RootParamIndex = static_cast<uint32_t>(-1);
                    desc[i].OfflineDescriptors = nullptr;
                    desc[i].NumDescriptors = 0;
                }

                desc[i].IsDirty = cache.IsDirty();
                cache.SetDirty(false);
            }
        };

        GfxRootDescriptorTableDesc srvUav[NumProgramTypes]{};
        set(srvUav, &RootSignatureType::GetSrvUavTableRootParamIndex, m_SrvUavCache);

        GfxRootDescriptorTableDesc samplers[NumProgramTypes]{};
        set(samplers, &RootSignatureType::GetSamplerTableRootParamIndex, m_SamplerCache);

        cmd->SetRootDescriptorTables<_PipelineType>(srvUav, samplers);
    }
}
