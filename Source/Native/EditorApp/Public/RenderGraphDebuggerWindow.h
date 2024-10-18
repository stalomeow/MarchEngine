#pragma once

#include "EditorWindow.h"
#include "RenderGraph.h"

namespace march
{
    class RenderGraphDebuggerWindow : public EditorWindow, public IRenderGraphCompiledEventListener
    {
        using base = typename EditorWindow;

    public:
        RenderGraphDebuggerWindow() = default;
        virtual ~RenderGraphDebuggerWindow() = default;

        void OnGraphCompiled(const RenderGraph& graph, const std::vector<int32_t>& sortedPasses) override;

    protected:
        void OnOpen() override;
        void OnClose() override;
        void OnDraw() override;
    };
}
