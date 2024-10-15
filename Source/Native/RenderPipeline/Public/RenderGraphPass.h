#pragma once

#include "GfxTexture.h"
#include <string>
#include <stdint.h>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace march
{
    class RenderGraph;
    class RenderGraphBuilder;

    enum class RenderGraphPassSortState
    {
        None,
        Visiting,
        Visited,
        Culled,
    };

    class RenderGraphPass
    {
        friend RenderGraph;
        friend RenderGraphBuilder;

    protected:
        RenderGraphPass(const std::string& name);
        virtual ~RenderGraphPass() = default;

        const std::string& GetName() const;

        virtual void OnSetup(RenderGraphBuilder& builder) = 0;
        virtual void OnExecute() = 0;

    private:
        std::string m_Name;

        bool m_AllowPassCulling;
        std::unordered_set<int32_t> m_ResourcesWritten; // 相当于出去的边
        std::unordered_set<int32_t> m_ResourcesRead;    // 相当于进来的边
        std::unordered_map<int32_t, GfxRenderTextureDesc> m_TexturesCreated;

        RenderGraphPassSortState m_SortState;
        std::vector<RenderGraphPass*> m_NextPasses; // 后继结点
        std::vector<int32_t> m_ResourcesBorn;       // 生命周期从本结点开始的资源
        std::vector<int32_t> m_ResourcesDead;       // 生命周期到本结点结束的资源
    };

    class RGTextureHandle final
    {
    public:
        RGTextureHandle(RenderGraph* graph, int32_t resourceId);
        ~RGTextureHandle() = default;

        GfxRenderTexture* GetTexture();

    private:
        RenderGraph* m_Graph;
        int32_t m_ResourceId;
    };

    class RenderGraphBuilder final
    {
    public:
        RenderGraphBuilder(RenderGraph* graph, RenderGraphPass* pass);
        ~RenderGraphBuilder() = default;

        void AllowPassCulling(bool value);

        void CreateTexture(int32_t id, const GfxRenderTextureDesc& desc);
        RGTextureHandle ReadTexture(int32_t id);
        RGTextureHandle WriteTexture(int32_t id);

    private:
        RenderGraph* m_Graph;
        RenderGraphPass* m_Pass;
    };
}
