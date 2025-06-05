#pragma once

#include "EditorWindow.h"

namespace march
{
    class GraphicsDebuggerWindow : public EditorWindow
    {
        using base = typename EditorWindow;

        void DrawOnlineViewDescriptorAllocatorInfo();
        void DrawOnlineSamplerDescriptorAllocatorInfo();

    protected:
        void OnDraw() override;
    };
}
