#pragma once

#include "Engine/Ints.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderGraphImpl/RenderGraphResource.h"
#include <directx/d3d12.h>
#include <vector>
#include <unordered_set>
#include <memory>
#include <string>
#include <optional>
#include <functional>
#include <DirectXColors.h>

namespace march
{
    class RenderGraph;
    class RenderGraphContext;

    enum class RenderTargetInitMode
    {
        Load,
        Discard,
        Clear,
    };

    struct RenderGraphPassColorTarget
    {
        size_t ResourceIndex = 0;
        bool IsSet = false;

        RenderTargetInitMode InitMode = RenderTargetInitMode::Load;
        float ClearColor[4]{};
    };

    struct RenderGraphPassDepthStencilTarget
    {
        size_t ResourceIndex = 0;
        bool IsSet = false;

        RenderTargetInitMode InitMode = RenderTargetInitMode::Load;
        float ClearDepthValue = GfxUtils::FarClipPlaneDepth;
        uint8_t ClearStencilValue = 0;
    };

    struct RenderGraphPass final
    {
        // -----------------------
        // Config Data
        // -----------------------

        std::string Name{};

        bool HasSideEffects = false; // 如果写入了 external resource，就有副作用
        bool AllowPassCulling = true;
        bool EnableAsyncCompute = false; // 只是一个建议，会根据实际情况决定是否启用 async-compute

        std::unordered_set<size_t> ResourcesIn{};  // 所有输入的资源，包括 render target
        std::unordered_set<size_t> ResourcesOut{}; // 所有输出的资源，包括 render target

        uint32_t NumColorTargets = 0;
        RenderGraphPassColorTarget ColorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        RenderGraphPassDepthStencilTarget DepthStencilTarget{};

        bool HasCustomViewport = false;
        D3D12_VIEWPORT CustomViewport{};

        bool HasCustomScissorRect = false;
        D3D12_RECT CustomScissorRect{};

        bool HasCustomDepthBias = false;
        int32_t DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        float DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        float SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

        bool Wireframe = false;

        std::function<void(RenderGraphContext&)> RenderFunc{};

        // -----------------------
        // Runtime Data
        // -----------------------

        bool IsVisited = false;
        bool IsCulled = false;

        std::unordered_set<size_t> NextPassIndices{}; // 后继结点
        std::vector<size_t> ResourcesBorn{};          // 生命周期从本结点开始的资源
        std::vector<size_t> ResourcesDead{};          // 生命周期到本结点结束的资源

        bool IsAsyncCompute = false;
        GfxSyncPoint SyncPoint{};                             // 如果当前 pass 是 async-compute，则会产生一个 sync point
        std::optional<size_t> PassIndexToWait = std::nullopt; // 在执行当前 pass 前需要等待的 pass，利用 sync point 实现
    };

    class RenderGraphBuilder final
    {
        RenderGraph* m_Graph;
        size_t m_PassIndex;

        RenderGraphPass& GetPass() const;

        void InResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex);
        void OutResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex);

    public:
        RenderGraphBuilder(RenderGraph* graph, size_t passIndex) : m_Graph(graph), m_PassIndex(passIndex) {}

        void AllowPassCulling(bool value);
        void EnableAsyncCompute(bool value);

        // 表示需要读取前面 pass 写入 buffer 的数据
        void In(const BufferHandle& buffer);

        // 表示需要写入 buffer，当前和之后的 pass 可以读取
        void Out(const BufferHandle& buffer);

        // Shortcut for In and Out
        void InOut(const BufferHandle& buffer);

        // 表示需要读取前面 pass 写入 texture 的数据
        void In(const TextureHandle& texture);

        // 表示需要写入 texture，当前和之后的 pass 可以读取
        void Out(const TextureHandle& texture);

        // Shortcut for In and Out
        void InOut(const TextureHandle& texture);

        void SetColorTarget(const TextureHandle& texture, RenderTargetInitMode initMode, const float color[4] = DirectX::Colors::Black);
        void SetColorTarget(const TextureHandle& texture, uint32_t index = 0, RenderTargetInitMode initMode = RenderTargetInitMode::Load, const float color[4] = DirectX::Colors::Black);
        void SetDepthStencilTarget(const TextureHandle& texture, RenderTargetInitMode initMode = RenderTargetInitMode::Load, float depth = GfxUtils::FarClipPlaneDepth, uint8_t stencil = 0);

        void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

        void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

        void SetDepthBias(int32_t bias, float slopeScaledBias, float clamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP);

        void SetWireframe(bool value);

        void SetRenderFunc(const std::function<void(RenderGraphContext&)>& func);
    };

    class IRenderGraphCompiledEventListener
    {
    public:
        virtual void OnGraphCompiled(const std::vector<RenderGraphPass>& passes, const RenderGraphResourceManager* resourceManager) = 0;
    };

    class RenderGraph final
    {
        friend RenderGraphBuilder;

        std::vector<RenderGraphPass> m_Passes{};
        std::optional<size_t> m_PassIndexToWaitFallback = std::nullopt; // 用于等待不被任何 pass 依赖的 async compute 结束
        std::unique_ptr<RenderGraphResourceManager> m_ResourceManager = std::make_unique<RenderGraphResourceManager>();

        void CompilePasses();
        void CullPass(size_t passIndex, size_t& asyncComputeDeadlineIndexExclusive);
        void CompileAsyncCompute(size_t passIndex, size_t& deadlineIndexExclusive);
        size_t AvoidAsyncComputeResourceHazard(size_t passIndex, size_t& deadlineIndexExclusive);

        void RequestPassResources(const RenderGraphPass& pass);
        void EnsureAsyncComputePassResourceStates(RenderGraphContext& context, const RenderGraphPass& pass);
        GfxCommandContext* EnsurePassContext(RenderGraphContext& context, const RenderGraphPass& pass);
        void SetPassRenderStates(GfxCommandContext* cmd, const RenderGraphPass& pass);
        void ReleasePassResources(const RenderGraphPass& pass);
        void ExecutePasses();

        class DeferredCleanup
        {
            RenderGraph* m_Graph;

        public:
            DeferredCleanup(RenderGraph* graph) : m_Graph(graph) {}

            ~DeferredCleanup()
            {
                m_Graph->m_Passes.clear();
                m_Graph->m_PassIndexToWaitFallback = std::nullopt;
                m_Graph->m_ResourceManager->ClearResources();
            }
        };

    public:
        RenderGraphBuilder AddPass();
        RenderGraphBuilder AddPass(const std::string& name);
        void CompileAndExecute();

        BufferHandle ImportBuffer(const std::string& name, GfxBuffer* buffer);
        BufferHandle ImportBuffer(int32 id, GfxBuffer* buffer);

        BufferHandle RequestBuffer(const std::string& name, const GfxBufferDesc& desc);
        BufferHandle RequestBuffer(int32 id, const GfxBufferDesc& desc);

        BufferHandle RequestBufferWithContent(const std::string& name, const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter = std::nullopt);
        BufferHandle RequestBufferWithContent(int32 id, const GfxBufferDesc& desc, const void* pData, std::optional<uint32_t> counter = std::nullopt);

        TextureHandle ImportTexture(const std::string& name, GfxTexture* texture);
        TextureHandle ImportTexture(int32 id, GfxTexture* texture);

        TextureHandle RequestTexture(const std::string& name, const GfxTextureDesc& desc);
        TextureHandle RequestTexture(int32 id, const GfxTextureDesc& desc);

        static void AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
        static void RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
    };

    class RenderGraphContext final
    {
        friend RenderGraph;

        GfxCommandContext* m_Cmd = nullptr;

        void New(GfxCommandType type, bool waitPreviousOneOnGpu);
        void Ensure(GfxCommandType type);

        GfxSyncPoint UncheckedSubmit();
        void Submit();

    public:
        void SetVariable(const TextureHandle& texture, GfxTextureElement element = GfxTextureElement::Default, uint32_t unorderedAccessMipSlice = 0);
        void SetVariable(const TextureHandle& texture, const std::string& aliasName, GfxTextureElement element = GfxTextureElement::Default, uint32_t unorderedAccessMipSlice = 0);
        void SetVariable(const TextureHandle& texture, int32 aliasId, GfxTextureElement element = GfxTextureElement::Default, uint32_t unorderedAccessMipSlice = 0);

        void SetVariable(const BufferHandle& buffer, GfxBufferElement element = GfxBufferElement::StructuredData);
        void SetVariable(const BufferHandle& buffer, const std::string& aliasName, GfxBufferElement element = GfxBufferElement::StructuredData);
        void SetVariable(const BufferHandle& buffer, int32 aliasId, GfxBufferElement element = GfxBufferElement::StructuredData);

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex)
        {
            m_Cmd->DrawMesh(geometry, material, shaderPassIndex);
        }

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix)
        {
            m_Cmd->DrawMesh(geometry, material, shaderPassIndex, matrix);
        }

        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex)
        {
            m_Cmd->DrawMesh(mesh, subMeshIndex, material, shaderPassIndex);
        }

        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix)
        {
            m_Cmd->DrawMesh(mesh, subMeshIndex, material, shaderPassIndex, matrix);
        }

        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex)
        {
            m_Cmd->DrawMesh(subMesh, material, shaderPassIndex);
        }

        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix)
        {
            m_Cmd->DrawMesh(subMesh, material, shaderPassIndex, matrix);
        }

        void DrawMeshRenderers(size_t numRenderers, MeshRenderer* const* renderers, const std::string& lightMode)
        {
            m_Cmd->DrawMeshRenderers(numRenderers, renderers, lightMode);
        }

        void DispatchCompute(ComputeShader* shader, size_t kernelIndex, uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ)
        {
            m_Cmd->DispatchCompute(shader, kernelIndex, threadGroupCountX, threadGroupCountY, threadGroupCountZ);
        }

        void ResolveTexture(GfxTexture* source, GfxTexture* destination)
        {
            m_Cmd->ResolveTexture(source, destination);
        }

        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement)
        {
            m_Cmd->CopyBuffer(sourceBuffer, sourceElement, destinationBuffer, destinationElement);
        }

        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, uint32_t sourceOffsetInBytes, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement, uint32_t destinationOffsetInBytes, uint32_t sizeInBytes)
        {
            m_Cmd->CopyBuffer(sourceBuffer, sourceElement, sourceOffsetInBytes, destinationBuffer, destinationElement, destinationOffsetInBytes, sizeInBytes);
        }

        void UpdateSubresources(RefCountPtr<GfxResource> destination, uint32_t firstSubresource, uint32_t numSubresources, const D3D12_SUBRESOURCE_DATA* srcData)
        {
            m_Cmd->UpdateSubresources(destination, firstSubresource, numSubresources, srcData);
        }

        GfxDevice* GetDevice() const { return m_Cmd->GetDevice(); }
        GfxCommandContext* GetCommandContext() const { return m_Cmd; }
    };
}
