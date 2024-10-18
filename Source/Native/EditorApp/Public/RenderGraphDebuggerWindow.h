#pragma once

#include "EditorWindow.h"
#include "RenderGraph.h"
#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <imgui.h>

namespace march
{
    struct RenderPassNode
    {
        std::string Name;
        std::vector<std::string> Resources;

        RenderPassNode(const std::string& name);
    };

    struct RenderPassLink
    {
        int32_t SrcNodeIndex;
        int32_t SrcResourceIndex;
        int32_t DstNodeIndex;
        int32_t DstResourceIndex;

        RenderPassLink(int32_t srcNodeIndex, int32_t srcResourceIndex, int32_t dstNodeIndex, int32_t dstResourceIndex);
    };

    struct RenderPassNodeState
    {
        ImVec2 Position{};
        ImVec2 Size{};
    };

    class RenderGraphDebuggerWindow : public EditorWindow, public IRenderGraphCompiledEventListener
    {
        using base = typename EditorWindow;

    public:
        RenderGraphDebuggerWindow() = default;
        virtual ~RenderGraphDebuggerWindow() = default;

        void OnGraphCompiled(const RenderGraph& graph, const std::vector<int32_t>& sortedPasses) override;

    protected:
        bool Begin() override;
        void OnOpen() override;
        void OnClose() override;
        void OnDraw() override;

    private:
        void DrawSidebar();
        void DrawCanvas();
        void DrawGrid(ImDrawList* drawList) const;
        void DrawLinks(ImDrawList* drawList, const ImVec2& offset) const;
        ImVec2 GetLinkSrcPos(int32_t nodeIndex, int32_t resourceIndex) const;
        ImVec2 GetLinkDstPos(int32_t nodeIndex, int32_t resourceIndex) const;
        void DrawNodes(ImDrawList* drawList, const ImVec2& offset);

        std::vector<RenderPassNode> m_Nodes{};
        std::vector<RenderPassLink> m_Links{};

        ImVec2 m_ScrollPos{};
        int32_t m_SelectedNodeIndex = -1;
        int32_t m_HoveredNodeIndex = -1;
        std::unordered_map<std::string, RenderPassNodeState> m_NodeStates{};

        const ImVec2 m_NodeWindowPadding = ImVec2(8.0f, 8.0f);
    };
}
