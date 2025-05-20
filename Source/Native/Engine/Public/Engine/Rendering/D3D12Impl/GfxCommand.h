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

namespace march
{
    class GfxDevice;
    class GfxResource;
    class Material;
    class MeshRenderer;
    class GfxMesh;
    struct GfxSubMeshDesc;
    enum class GfxMeshGeometry;

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

        uint64_t GetCompletedFrameFence() const;
        bool IsFrameFenceCompleted(uint64_t fence) const;
        uint64_t GetNextFrameFence() const;
        void SignalNextFrameFence(bool waitForGpuIdle);

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        struct QueueData
        {
            std::unique_ptr<GfxCommandQueue> Queue;
            std::unique_ptr<GfxFence> FrameFence;
            std::queue<GfxCommandContext*> FreeContexts;
        };

        QueueData m_QueueData[static_cast<size_t>(GfxCommandType::NumTypes)];

        GfxDevice* m_Device;
        std::vector<std::unique_ptr<GfxCommandContext>> m_ContextStore; // 保存所有分配的 command context，用于释放资源
        uint64_t m_CompletedFrameFence; // cache
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

    // 不要跨帧使用
    class GfxCommandContext final
    {
    public:
        GfxCommandContext(GfxDevice* device, GfxCommandType type);
        ~GfxCommandContext();

        void Open();
        GfxSyncPoint SubmitAndRelease();

        void BeginEvent(const std::string& name);
        void EndEvent();

        void TransitionResource(RefCountPtr<GfxResource> resource, D3D12_RESOURCE_STATES stateAfter);
        void TransitionSubresource(RefCountPtr<GfxResource> resource, uint32_t subresource, D3D12_RESOURCE_STATES stateAfter);
        void FlushResourceBarriers();

        void WaitOnGpu(const GfxSyncPoint& syncPoint);

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
        GfxCommandType GetType() const { return m_Type; }

    private:
        GfxDevice* m_Device;
        GfxCommandType m_Type;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

        std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;
        std::vector<GfxSyncPoint> m_SyncPointsToWait;

        GfxPipelineParameterCache<GfxPipelineType::Graphics> m_GraphicsViewCache;
        GfxPipelineParameterCache<GfxPipelineType::Compute> m_ComputeViewCache;

        GfxDescriptorHeap* m_ViewHeap;
        GfxDescriptorHeap* m_SamplerHeap;

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

        void* m_NsightAftermathHandle;

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

        template <GfxPipelineType _PipelineType>
        struct SetRootDescriptorTables
        {
            size_t OfflineDescriptorOffset;

            size_t OfflineSrvUavTableDataOffset;
            uint32_t TotalNumOfflineSrvUav;

            size_t OfflineSamplerTableDataOffset;
            uint32_t TotalNumOfflineSamplers;
        };
    }

    struct GfxRootDescriptorTableDesc
    {
        const D3D12_CPU_DESCRIPTOR_HANDLE* OfflineDescriptors;
        uint32_t NumDescriptors;
        bool IsDirty;
    };

    class GfxCommandList final
    {
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
            GfxCommands::SetRootDescriptorTables<GfxPipelineType::Graphics>,
            GfxCommands::SetRootDescriptorTables<GfxPipelineType::Compute>
        >;

        std::vector<CommandType> m_Commands;

        std::vector<D3D12_RESOURCE_BARRIER> m_ResourceBarriers;
        size_t m_ResourceBarrierFlushOffset;

        // 统一保存 SetRenderTargets / SetViewports / SetScissorRects 的参数，避免频繁创建销毁 vector
        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_ColorTargets;
        std::vector<D3D12_VIEWPORT> m_Viewports;
        std::vector<D3D12_RECT> m_ScissorRects;

        struct OfflineDescriptorTableData
        {
            uint32_t Num;
            bool IsDirty;
        };

        std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_OfflineDescriptors;
        std::vector<OfflineDescriptorTableData> m_OfflineDescriptorTableData;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_List;

        void* m_NsightAftermathHandle;

    public:
        void BeginEvent(const std::string& name);
        void BeginEvent(std::string&& name);
        void EndEvent();

        void AddResourceBarrier(const D3D12_RESOURCE_BARRIER& barrier);
        void FlushResourceBarriers();

        void SetRenderTargets(uint32_t numColorTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* colorTargets, const D3D12_CPU_DESCRIPTOR_HANDLE* depthStencilTarget);
        void ClearColorTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, float color[4]);
        void ClearDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE target, D3D12_CLEAR_FLAGS flags, float depth, uint8_t stencil);

        void SetViewports(uint32_t num, const D3D12_VIEWPORT* viewports);
        void SetScissorRects(uint32_t num, const D3D12_RECT* rects);

        void SetPredication(ID3D12Resource* buffer, uint32_t alignedOffset, D3D12_PREDICATION_OP operation);

        void SetPipelineState(ID3D12PipelineState* state);

        template <GfxPipelineType _PipelineType>
        void SetRootDescriptorTables(
            const GfxRootDescriptorTableDesc(&srvUav)[GfxPipelineTraits<_PipelineType>::NumProgramTypes],
            const GfxRootDescriptorTableDesc(&samplers)[GfxPipelineTraits<_PipelineType>::NumProgramTypes]);

    private:
        void TranslateCommand(const GfxCommands::BeginEvent& cmd);
        void TranslateCommand(const GfxCommands::EndEvent& cmd);
        void TranslateCommand(const GfxCommands::FlushResourceBarriers& cmd);
        void TranslateCommand(const GfxCommands::SetRenderTargets& cmd);
        void TranslateCommand(const GfxCommands::ClearColorTarget& cmd);
        void TranslateCommand(const GfxCommands::ClearDepthStencilTarget& cmd);
        void TranslateCommand(const GfxCommands::SetViewports& cmd);
        void TranslateCommand(const GfxCommands::SetScissorRects& cmd);
        void TranslateCommand(const GfxCommands::SetPredication& cmd);
        void TranslateCommand(const GfxCommands::SetPipelineState& cmd);
    };

    template <GfxPipelineType _PipelineType>
    void GfxCommandList::SetRootDescriptorTables(
        const GfxRootDescriptorTableDesc(&srvUav)[GfxPipelineTraits<_PipelineType>::NumProgramTypes],
        const GfxRootDescriptorTableDesc(&samplers)[GfxPipelineTraits<_PipelineType>::NumProgramTypes])
    {
        using MyCommand = GfxCommands::SetRootDescriptorTables<_PipelineType>;
        using PipelineTraits = GfxPipelineTraits<_PipelineType>;
        constexpr size_t NumProgramTypes = PipelineTraits::NumProgramTypes;

        auto set = [this](const GfxRootDescriptorTableDesc(&desc)[NumProgramTypes], size_t& tableDataOffset, uint32_t& totalNum)
        {
            tableDataOffset = m_OfflineDescriptorTableData.size();
            totalNum = 0;

            for (size_t i = 0; i < NumProgramTypes; i++)
            {
                OfflineDescriptorTableData& data = m_OfflineDescriptorTableData.emplace_back();
                data.Num = desc[i].NumDescriptors;
                data.IsDirty = desc[i].IsDirty;

                const D3D12_CPU_DESCRIPTOR_HANDLE* descriptors = desc[i].OfflineDescriptors;
                m_OfflineDescriptors.insert(m_OfflineDescriptors.end(), descriptors, descriptors + data.Num);

                totalNum += data.Num;
            }
        };

        MyCommand cmd{};
        cmd.OfflineDescriptorOffset = m_OfflineDescriptors.size();
        set(srvUav, cmd.OfflineSrvUavTableDataOffset, cmd.TotalNumOfflineSrvUav);
        set(samplers, cmd.OfflineSamplerTableDataOffset, cmd.TotalNumOfflineSamplers);
        m_Commands.emplace_back(cmd);
    }
}
