#pragma once

#include "Engine/Ints.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderGraphImpl/RenderGraphResource.h"
#include <directx/d3d12.h>
#include <vector>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include <DirectXColors.h>

namespace march
{
    class RenderGraph;
    class RenderGraphContext;

    enum class RenderGraphPassState
    {
        None,
        Visiting,
        Visited,
        Culled,
    };

    struct RenderGraphPassRenderTarget
    {
        size_t ResourceIndex = 0;
        bool IsSet = false;
        bool Load = false;
    };

    struct RenderGraphPass final
    {
        std::string Name{};

        bool HasSideEffects = false; // 如果写入了 external resource，就有副作用
        bool AllowPassCulling = true;
        bool EnableAsyncCompute = false;

        std::unordered_set<size_t> ResourcesRead{};    // 所有读取的资源，包括 render target
        std::unordered_set<size_t> ResourcesWritten{}; // 所有写入的资源，包括 render target

        uint32_t NumColorTargets = 0;
        RenderGraphPassRenderTarget ColorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT]{};
        RenderGraphPassRenderTarget DepthStencilTarget{};

        GfxClearFlags RenderTargetsClearFlags = GfxClearFlags::None;
        float ClearColorValue[4]{};
        float ClearDepthValue = GfxUtils::FarClipPlaneDepth;
        uint8_t ClearStencilValue = 0;

        bool HasCustomViewport = false;
        D3D12_VIEWPORT CustomViewport{};

        bool HasCustomScissorRect = false;
        D3D12_RECT CustomScissorRect{};

        bool HashCustomDepthBias = false;
        int32_t DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        float DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        float SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;

        bool Wireframe = false;

        std::function<void(RenderGraphContext&)> RenderFunc{};

        RenderGraphPassState State = RenderGraphPassState::None;
        std::vector<size_t> NextPassIndices{}; // 后继结点
        std::vector<size_t> ResourcesBorn{};   // 生命周期从本结点开始的资源
        std::vector<size_t> ResourcesDead{};   // 生命周期到本结点结束的资源
    };

    class RenderGraphBuilder final
    {
        RenderGraph* m_Graph;
        size_t m_PassIndex;

        RenderGraphPass& GetPass() const;
        void ReadResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex);
        void WriteResource(RenderGraphResourceManager* resourceManager, size_t resourceIndex);

    public:
        RenderGraphBuilder(RenderGraph* graph, size_t passIndex) : m_Graph(graph), m_PassIndex(passIndex) {}

        void AllowPassCulling(bool value);
        void EnableAsyncCompute(bool value);

        void Read(const BufferHandle& buffer);
        void Write(const BufferHandle& buffer);
        void ReadWrite(const BufferHandle& buffer);

        void Read(const TextureHandle& texture);
        void Write(const TextureHandle& texture);
        void ReadWrite(const TextureHandle& texture);

        void SetColorTarget(const TextureHandle& texture, bool load);
        void SetColorTarget(const TextureHandle& texture, uint32_t index = 0, bool load = true);
        void SetDepthStencilTarget(const TextureHandle& texture, bool load = true);
        void ClearRenderTargets(GfxClearFlags flags = GfxClearFlags::All, const float color[4] = DirectX::Colors::Black, float depth = GfxUtils::FarClipPlaneDepth, uint8_t stencil = 0);

        void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);

        void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

        void SetDepthBias(int32_t bias, float slopeScaledBias, float clamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP);

        void SetWireframe(bool value);

        void SetRenderFunc(const std::function<void(RenderGraphContext&)>& func);
    };

    class IRenderGraphCompiledEventListener
    {
    public:
        virtual void OnGraphCompiled(const RenderGraph& graph, const std::vector<RenderGraphPass>& passes) = 0;
    };

    class RenderGraph final
    {
        friend RenderGraphBuilder;

        std::vector<RenderGraphPass> m_Passes{};
        std::unique_ptr<RenderGraphResourceManager> m_ResourceManager = std::make_unique<RenderGraphResourceManager>();

        bool CullPasses();
        bool CullPassDFS(size_t passIndex);
        void ComputeResourceLifetime();
        bool CompilePasses();

        void SetPassRenderTargets(GfxCommandContext* context, RenderGraphPass& pass);
        void ExecutePasses();

    public:
        RenderGraphBuilder AddPass();
        RenderGraphBuilder AddPass(const std::string& name);
        void CompileAndExecute();

        BufferHandle Import(const std::string& name, GfxBuffer* buffer);
        BufferHandle Import(int32 id, GfxBuffer* buffer);

        BufferHandle Request(const std::string& name, const GfxBufferDesc& desc);
        BufferHandle Request(int32 id, const GfxBufferDesc& desc);

        TextureHandle Import(const std::string& name, GfxRenderTexture* texture);
        TextureHandle Import(int32 id, GfxRenderTexture* texture);

        TextureHandle Request(const std::string& name, const GfxTextureDesc& desc);
        TextureHandle Request(int32 id, const GfxTextureDesc& desc);

        static void AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
        static void RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
    };

    class RenderGraphContext final
    {
        friend class RenderGraph;

    public:
        RenderGraphContext();
        ~RenderGraphContext();

        void SetTexture(const std::string& name, GfxTexture* value) { m_Context->SetTexture(name, value); }
        void SetTexture(int32_t id, GfxTexture* value) { m_Context->SetTexture(id, value); }
        void SetBuffer(const std::string& name, GfxBuffer* value) { m_Context->SetBuffer(name, value); }
        void SetBuffer(int32_t id, GfxBuffer* value) { m_Context->SetBuffer(id, value); }

        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex) { m_Context->DrawMesh(geometry, material, shaderPassIndex); }
        void DrawMesh(GfxMeshGeometry geometry, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix) { m_Context->DrawMesh(geometry, material, shaderPassIndex, matrix); }
        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex) { m_Context->DrawMesh(mesh, subMeshIndex, material, shaderPassIndex); }
        void DrawMesh(GfxMesh* mesh, uint32_t subMeshIndex, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix) { m_Context->DrawMesh(mesh, subMeshIndex, material, shaderPassIndex, matrix); }
        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex) { m_Context->DrawMesh(subMesh, material, shaderPassIndex); }
        void DrawMesh(const GfxSubMeshDesc& subMesh, Material* material, size_t shaderPassIndex, const DirectX::XMFLOAT4X4& matrix) { m_Context->DrawMesh(subMesh, material, shaderPassIndex, matrix); }
        void DrawMeshRenderers(size_t numRenderers, MeshRenderer* const* renderers, const std::string& lightMode) { m_Context->DrawMeshRenderers(numRenderers, renderers, lightMode); }

        void ResolveTexture(GfxTexture* source, GfxTexture* destination) { m_Context->ResolveTexture(source, destination); }
        void CopyBuffer(GfxBuffer* sourceBuffer, GfxBufferElement sourceElement, GfxBuffer* destinationBuffer, GfxBufferElement destinationElement) { m_Context->CopyBuffer(sourceBuffer, sourceElement, destinationBuffer, destinationElement); }

        GfxDevice* GetDevice() const { return m_Context->GetDevice(); }
        GfxCommandContext* GetCommandContext() const { return m_Context; }

        RenderGraphContext(const RenderGraphContext&) = delete;
        RenderGraphContext& operator=(const RenderGraphContext&) = delete;

        RenderGraphContext(RenderGraphContext&&) = delete;
        RenderGraphContext& operator=(RenderGraphContext&&) = delete;

    private:
        GfxCommandContext* m_Context;

        void ClearPassData();
    };
}
