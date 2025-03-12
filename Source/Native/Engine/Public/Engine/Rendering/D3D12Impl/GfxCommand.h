#pragma once

#include "Engine/Memory/RefCounting.h"
#include "Engine/Rendering/D3D12Impl/GfxDescriptor.h"
#include "Engine/Rendering/D3D12Impl/GfxPipeline.h"
#include "Engine/Rendering/D3D12Impl/GfxBuffer.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Rendering/D3D12Impl/ShaderGraphics.h"
#include "Engine/Rendering/D3D12Impl/ShaderCompute.h"
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

        void RefreshCompletedFrameFence();
        uint64_t GetCompletedFrameFence() const;
        bool IsFrameFenceCompleted(uint64_t fence) const;
        uint64_t GetNextFrameFence() const;

        void SignalNextFrameFence();
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

        void Open();
        GfxSyncPoint SubmitAndRelease();

        void BeginEvent(const std::string& name);
        void EndEvent();

        void TransitionResource(RefCountPtr<GfxResource> resource, D3D12_RESOURCE_STATES stateAfter);
        void FlushResourceBarriers();

        void WaitOnGpu(const GfxSyncPoint& syncPoint);

        void SetTexture(const std::string& name, GfxTexture* value, GfxTextureElement element = GfxTextureElement::Default, uint32_t unorderedAccessMipSlice = 0);
        void SetTexture(int32_t id, GfxTexture* value, GfxTextureElement element = GfxTextureElement::Default, uint32_t unorderedAccessMipSlice = 0);
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

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex);
        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix);
        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex);
        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix);
        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex);
        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix);

        void DrawMeshRenderers(size_t numRenderers, MeshRenderer* const* renderers, const std::string& lightMode);

        void DispatchCompute(ComputeShader* shader, size_t kernelIndex, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ);
        void DispatchComputeByThreadCount(ComputeShader* shader, size_t kernelIndex, uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ);

        void ResolveTexture(GfxTexture* source, GfxTexture* destination);
        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement);
        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, uint32_t sourceOffsetInBytes, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement, uint32_t destinationOffsetInBytes, uint32_t sizeInBytes);
        void UpdateSubresources(RefCountPtr<GfxResource> destination, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData);

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

        ID3D12PipelineState* m_CurrentPipelineState;

        D3D12_PRIMITIVE_TOPOLOGY m_CurrentPrimitiveTopology;
        D3D12_VERTEX_BUFFER_VIEW m_CurrentVertexBuffer;
        D3D12_INDEX_BUFFER_VIEW m_CurrentIndexBuffer;
        std::optional<uint8_t> m_CurrentStencilRef;

        struct GlobalTextureData
        {
            GfxTexture* Texture;
            GfxTextureElement Element;
            uint32_t UnorderedAccessMipSlice;
        };

        struct GlobalBufferData
        {
            GfxBuffer* Buffer;
            GfxBufferElement Element;
        };

        std::unordered_map<int32_t, GlobalTextureData> m_GlobalTextures;
        std::unordered_map<int32_t, GlobalBufferData> m_GlobalBuffers;

        struct InstanceData
        {
            DirectX::XMFLOAT4X4 Matrix;
            DirectX::XMFLOAT4X4 MatrixIT; // 逆转置，用于法线变换
            DirectX::XMFLOAT4X4 MatrixPrev; // 上一帧的矩阵
        };

        GfxBuffer m_InstanceBuffer;

        void SetRenderTargets(uint32_t numColorTargets, const GfxRenderTargetDesc* colorTargets, const GfxRenderTargetDesc* depthStencilTarget);
        static D3D12_CPU_DESCRIPTOR_HANDLE GetRtvDsvFromRenderTargetDesc(const GfxRenderTargetDesc& desc);

        GfxTexture* GetFirstRenderTarget() const;
        GfxTexture* FindTexture(int32_t id, GfxTextureElement* pOutElement, uint32_t* pOutUnorderedAccessMipSlice);
        GfxTexture* FindTexture(int32_t id, Material* material, GfxTextureElement* pOutElement, uint32_t* pOutUnorderedAccessMipSlice);
        GfxBuffer* FindComputeBuffer(int32_t id, bool isConstantBuffer, GfxBufferElement* pOutElement);
        GfxBuffer* FindGraphicsBuffer(int32_t id, bool isConstantBuffer, Material* material, size_t passIndex, GfxBufferElement* pOutElement);

        void SetGraphicsPipelineParameters(ID3D12PipelineState* pso, Material* material, size_t passIndex);
        void SetComputePipelineParameters(ID3D12PipelineState* pso, ComputeShader* shader, size_t kernelIndex);

        void SetResolvedRenderState(const ShaderPassRenderState& state);
        void SetStencilRef(uint8_t value);

        void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY value);
        void SetVertexBuffer(GfxBuffer* buffer);
        void SetIndexBuffer(GfxBuffer* buffer);
        void SetInstanceBufferData(uint32_t numInstances, const InstanceData* instances);
        void DrawSubMesh(const GfxSubMeshDesc& subMesh, uint32_t instanceCount);

        static InstanceData CreateInstanceData(const DirectX::XMFLOAT4X4& currMatrix, const DirectX::XMFLOAT4X4& prevMatrix);
    };
}
