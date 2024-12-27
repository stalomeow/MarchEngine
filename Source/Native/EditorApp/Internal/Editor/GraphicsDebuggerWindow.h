#pragma once

#include "Editor/EditorWindow.h"

namespace march
{
    class GraphicsDebuggerWindow : public EditorWindow
    {
        using base = typename EditorWindow;

    public:
        GraphicsDebuggerWindow() = default;
        virtual ~GraphicsDebuggerWindow() = default;

    protected:
        void OnDraw() override;
    };
}
