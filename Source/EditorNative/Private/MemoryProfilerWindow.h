#pragma once

#include "EditorWindow.h"

namespace march
{
    class MemoryProfilerWindow : public EditorWindow
    {
        using base = EditorWindow;

    protected:
        void OnDraw() override;
    };
}
