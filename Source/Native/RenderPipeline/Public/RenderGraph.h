#pragma once

#include "GfxTexture.h"
#include "GfxHelpers.h"
#include "RenderGraphResource.h"
#include "RenderGraphContext.h"
#include <directx/d3d12.h>
#include <vector>
#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <functional>
#include <DirectXColors.h>

namespace march
{
    using ReadFlags = typename RenderGraphResourceReadFlags;
    using WriteFlags = typename RenderGraphResourceWriteFlags;
    using ClearFlags = typename RenderTargetClearFlags;

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
        std::unordered_map<int32_t, ReadFlags> ResourcesRead;     // 相当于进来的边
        std::unordered_map<int32_t, WriteFlags> ResourcesWritten; // 相当于出去的边

        int32_t NumColorTargets;
        RenderTargetData ColorTargets[8];
        RenderTargetData DepthStencilTarget;

        ClearFlags RenderTargetsClearFlags;
        float ClearColorValue[4];
        float ClearDepthValue;
        uint8_t ClearStencilValue;

        bool HasCustomViewport;
        D3D12_VIEWPORT CustomViewport;

        bool HasCustomScissorRect;
        D3D12_RECT CustomScissorRect;

        RenderGraphPassSortState SortState;
        std::vector<int32_t> NextPasses;    // 后继结点
        std::vector<int32_t> ResourcesBorn; // 生命周期从本结点开始的资源
        std::vector<int32_t> ResourcesDead; // 生命周期到本结点结束的资源

        std::function<void(RenderGraphContext&)> RenderFunc;

        RenderGraphPass(const std::string& name);
        ~RenderGraphPass() = default;

        RenderGraphPass(const RenderGraphPass&) = delete;
        RenderGraphPass& operator=(const RenderGraphPass&) = delete;

        RenderGraphPass(RenderGraphPass&&) = default;
        RenderGraphPass& operator=(RenderGraphPass&&) = default;
    };

    class RenderGraphTextureHandle;
    class RenderGraphBuilder;

    class IRenderGraphCompiledEventListener
    {
    public:
        virtual void OnGraphCompiled(const RenderGraph& graph, const std::vector<int32_t>& sortedPasses) = 0;
    };

    class RenderGraph final
    {
        friend RenderGraphTextureHandle;
        friend RenderGraphBuilder;

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

        bool RentOrReturnResources(std::vector<int32_t> resourceIds, bool isReturn);
        void SetPassRenderTargets(RenderGraphPass& pass);
        void AddPassResourceBarriers(RenderGraphPass& pass);
        static D3D12_RESOURCE_STATES GetResourceReadState(RenderGraphResourceData& res, RenderGraphResourceReadFlags flags);
        static D3D12_RESOURCE_STATES GetResourceWriteState(RenderGraphResourceData& res, RenderGraphResourceWriteFlags flags);

        RenderGraphResourceData& GetResourceData(int32_t id);

        bool m_EmitEvents;
        std::vector<RenderGraphPass> m_Passes;
        std::vector<int32_t> m_SortedPasses;
        std::unordered_map<int32_t, RenderGraphResourceData> m_ResourceDataMap;
        std::unique_ptr<RenderGraphResourcePool> m_ResourcePool;
        std::unique_ptr<RenderGraphContext> m_Context;

        static std::unordered_set<IRenderGraphCompiledEventListener*> s_GraphCompiledEventListeners;
    };

    class RenderGraphTextureHandle final
    {
    public:
        RenderGraphTextureHandle();
        RenderGraphTextureHandle(RenderGraph* graph, int32_t resourceId);
        ~RenderGraphTextureHandle() = default;

        int32_t Id() const;
        GfxRenderTextureDesc GetDesc() const;
        GfxRenderTexture* Get() const;
        GfxRenderTexture* operator->() const;

    private:
        RenderGraph* m_Graph;
        int32_t m_ResourceId;
    };

    using TextureHandle = typename RenderGraphTextureHandle;

    class RenderGraphBuilder final
    {
        friend RenderGraph;

    public:
        void AllowPassCulling(bool value);

        void ImportTexture(int32_t id, GfxRenderTexture* texture);
        void CreateTransientTexture(int32_t id, const GfxRenderTextureDesc& desc);

        GfxRenderTextureDesc GetTextureDesc(int32_t id) const;
        TextureHandle ReadTexture(int32_t id, ReadFlags flags);
        TextureHandle WriteTexture(int32_t id, WriteFlags flags);

        void SetColorTarget(int32_t id, bool load);
        void SetColorTarget(int32_t id, int32_t index = 0, bool load = true);
        void SetDepthStencilTarget(int32_t id, bool load = true);
        void ClearRenderTargets(ClearFlags flags = ClearFlags::All, const float color[4] = DirectX::Colors::Black, float depth = GfxHelpers::GetFarClipPlaneDepth(), uint8_t stencil = 0);
        void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
        void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

        void SetRenderFunc(std::function<void(RenderGraphContext&)> func);

    private:
        RenderGraphBuilder(RenderGraph* graph, int32_t passIndex);
        RenderGraphPass& GetPass();

        RenderGraph* m_Graph;
        int32_t m_PassIndex;
    };
}
