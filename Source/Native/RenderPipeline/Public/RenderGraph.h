#pragma once

#include "GfxTexture.h"
#include "GfxSupportInfo.h"
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
    enum class RenderTargetLoadFlags
    {
        None = 0,
        DiscardColors = 1 << 0,
        DiscardDepthStencil = 1 << 1,
        DiscardAll = DiscardColors | DiscardDepthStencil,
    };

    RenderTargetLoadFlags operator|(RenderTargetLoadFlags lhs, RenderTargetLoadFlags rhs);
    RenderTargetLoadFlags& operator|=(RenderTargetLoadFlags& lhs, RenderTargetLoadFlags rhs);
    RenderTargetLoadFlags operator&(RenderTargetLoadFlags lhs, RenderTargetLoadFlags rhs);
    RenderTargetLoadFlags& operator&=(RenderTargetLoadFlags& lhs, RenderTargetLoadFlags rhs);

    using ReadFlags = typename RenderGraphResourceReadFlags;
    using WriteFlags = typename RenderGraphResourceWriteFlags;
    using ClearFlags = typename RenderTargetClearFlags;
    using LoadFlags = typename RenderTargetLoadFlags;

    enum class RenderGraphPassSortState
    {
        None,
        Visiting,
        Visited,
        Culled,
    };

    struct RenderGraphPass final
    {
        std::string Name;

        bool AllowPassCulling;
        std::unordered_map<int32_t, ReadFlags> ResourcesRead;     // 相当于进来的边
        std::unordered_map<int32_t, WriteFlags> ResourcesWritten; // 相当于出去的边

        bool HasRenderTargets;
        size_t NumColorTargets;
        int32_t ColorTargets[8];
        bool HasDepthStencilTarget;
        int32_t DepthStencilTarget;
        LoadFlags RenderTargetsLoadFlags;

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
        RenderGraph();
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
        RenderGraphTextureHandle(RenderGraph* graph, int32_t resourceId);
        ~RenderGraphTextureHandle() = default;

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
    public:
        RenderGraphBuilder(RenderGraph* graph, int32_t passIndex);
        ~RenderGraphBuilder() = default;

        void AllowPassCulling(bool value);

        void ImportTexture(int32_t id, GfxRenderTexture* texture);
        void CreateTransientTexture(int32_t id, const GfxRenderTextureDesc& desc);

        TextureHandle ReadTexture(int32_t id, ReadFlags flags);
        TextureHandle WriteTexture(int32_t id, WriteFlags flags);

        void SetRenderTargets(int32_t colorTarget, LoadFlags flags = LoadFlags::None);
        void SetRenderTargets(int32_t colorTarget, int32_t depthStencilTarget, LoadFlags flags = LoadFlags::None);
        void SetRenderTargets(size_t numColorTargets, const int32_t* colorTargets, LoadFlags flags = LoadFlags::None);
        void SetRenderTargets(size_t numColorTargets, const int32_t* colorTargets, int32_t depthStencilTarget, LoadFlags flags = LoadFlags::None);
        void ClearRenderTargets(ClearFlags flags = ClearFlags::All, const float color[4] = DirectX::Colors::Black, float depth = GfxSupportInfo::GetFarClipPlaneDepth(), uint8_t stencil = 0);
        void SetViewport(float topLeftX, float topLeftY, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f);
        void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

        void SetRenderFunc(std::function<void(RenderGraphContext&)> func);

    private:
        void PostSetRenderTargets();
        RenderGraphPass& GetPass();

        RenderGraph* m_Graph;
        int32_t m_PassIndex;
    };
}
