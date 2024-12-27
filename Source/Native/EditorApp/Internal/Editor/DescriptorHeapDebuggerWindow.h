#pragma once

#include "Editor/EditorWindow.h"
#include <string>

namespace march
{
    class GfxOnlineViewDescriptorAllocator;

    class DescriptorHeapDebuggerWindow : public EditorWindow
    {
        using base = typename EditorWindow;

    public:
        DescriptorHeapDebuggerWindow() = default;
        virtual ~DescriptorHeapDebuggerWindow() = default;

    protected:
        void OnDraw() override;

    private:
        void DrawHeapInfo(const std::string& name, GfxOnlineViewDescriptorAllocator* allocator);
    };
}
