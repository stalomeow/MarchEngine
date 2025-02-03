#pragma once

#include "Engine/Rendering/D3D12.h"
#include "Engine/Rendering/RenderGraphResource.h"
#include <directx/d3d12.h>
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include <optional>
#include <DirectXColors.h>

namespace march
{
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

    enum class RenderGraphPassSortState
    {
        None,
        Visiting,
        Visited,
        Culled,
    };

    struct RenderTargetData
    {
        int32_t Id;
        bool IsSet;
        bool Load;
    };

    struct RenderGraphPass final
    {
        std::string Name;

        bool HasSideEffects; // 如果写入了 persistent resource，那么就有副作用
        bool AllowPassCulling;
        bool EnableAsyncCompute;

        std::unordered_set<int32_t> ResourcesRead;    // 相当于进来的边
        std::unordered_set<int32_t> ResourcesWritten; // 相当于出去的边

        uint32_t NumColorTargets;
        RenderTargetData ColorTargets[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
        RenderTargetData DepthStencilTarget;

        GfxClearFlags RenderTargetsClearFlags;
        float ClearColorValue[4];
        float ClearDepthValue;
        uint8_t ClearStencilValue;

        std::optional<D3D12_VIEWPORT> CustomViewport;
        std::optional<D3D12_RECT> CustomScissorRect;

        bool HashCustomDepthBias;
        int32_t DepthBias;
        float DepthBiasClamp;
        float SlopeScaledDepthBias;

        bool Wireframe;

        RenderGraphPassSortState SortState;
        std::vector<int32_t> NextPasses;    // 后继结点
        std::vector<int32_t> ResourcesBorn; // 生命周期从本结点开始的资源
        std::vector<int32_t> ResourcesDead; // 生命周期到本结点结束的资源

        std::function<void(RenderGraphContext&)> RenderFunc;
    };

    class IRenderGraphCompiledEventListener
    {
    public:
        virtual void OnGraphCompiled(const RenderGraph& graph, const std::vector<int32_t>& sortedPasses) = 0;
    };

    class RenderGraph final
    {
        friend class TextureHandle;
        friend class RenderGraphBuilder;

    public:
        RenderGraph(bool emitEvents = true);
        ~RenderGraph() = default;

        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;

        RenderGraphBuilder AddPass();
        RenderGraphBuilder AddPass(const std::string& name);
        void CompileAndExecute();

        const RenderGraphPass& GetPass(int32_t index) const;
        int32_t GetPassCount() const;

        static void AddGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);
        static void RemoveGraphCompiledEventListener(IRenderGraphCompiledEventListener* listener);

    private:
        bool CompilePasses();
        void ExecutePasses();
        bool CullAndSortPasses();
        bool CullAndSortPassesDFS(int32_t passIndex);
        bool RecordResourceLifeTime();
        bool UpdateResourceLifeTime(int32_t sortedPassIndex, int32_t resourceId);

        bool RentOrReturnResources(const std::vector<int32_t>& resourceIds, bool isReturn);
        void SetPassRenderTargets(GfxCommandContext* context, RenderGraphPass& pass);

        RenderGraphResourceData& GetResourceData(int32_t id);

        bool m_EmitEvents;
        std::vector<RenderGraphPass> m_Passes;
        std::vector<int32_t> m_SortedPasses;
        std::unordered_map<int32_t, RenderGraphResourceData> m_ResourceDataMap;
        std::unique_ptr<RenderGraphResourcePool> m_ResourcePool;

        static std::unordered_set<IRenderGraphCompiledEventListener*> s_GraphCompiledEventListeners;
    };

    class TextureHandle final
    {
    public:
        TextureHandle(RenderGraph* graph, int32_t resourceId) : m_Graph(graph), m_ResourceId(resourceId) {}
        TextureHandle() : TextureHandle(nullptr, -1) {}

        const GfxTextureDesc& GetDesc() const { return m_Graph->GetResourceData(m_ResourceId).GetTextureDesc(); }
        GfxRenderTexture* Get() const { return m_Graph->GetResourceData(m_ResourceId).GetTexture(); }

        int32_t Id() const { return m_ResourceId; }
        GfxRenderTexture* operator->() const { return Get(); }

    private:
        RenderGraph* m_Graph;
        int32_t m_ResourceId;
    };

    class RenderGraphBuilder final
    {
        friend RenderGraph;

    public:
        void AllowPassCulling(bool value);

        void ImportTexture(int32_t id, GfxRenderTexture* texture);
        void CreateTransientTexture(int32_t id, const GfxTextureDesc& desc);

        const GfxTextureDesc& GetTextureDesc(int32_t id) const;
        TextureHandle ReadTexture(int32_t id);
        TextureHandle WriteTexture(int32_t id);

        void SetColorTarget(int32_t id, bool load);
        void SetColorTarget(int32_t id, uint32_t index = 0, bool load = true);
        void SetDepthStencilTarget(int32_t id, bool load = true);
        void ClearRenderTargets(GfxClearFlags flags = GfxClearFlags::All, const float color[4] = DirectX::Colors::Black, float depth = GfxUtils::FarClipPlaneDepth, uint8_t stencil = 0);
        void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
        void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
        void SetDepthBias(int32_t bias, float slopeScaledBias, float clamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP);
        void SetWireframe(bool value);

        void SetRenderFunc(const std::function<void(RenderGraphContext&)>& func);

    private:
        RenderGraphBuilder(RenderGraph* graph, int32_t passIndex);
        RenderGraphPass& GetPass();

        RenderGraph* m_Graph;
        int32_t m_PassIndex;
    };
}
