#pragma once

#include "EditorWindow.h"
#include <string>

namespace march
{
    class GfxDescriptorTableAllocator;

    class DescriptorHeapDebuggerWindow : public EditorWindow
    {
        using base = typename EditorWindow;

    public:
        DescriptorHeapDebuggerWindow() = default;
        virtual ~DescriptorHeapDebuggerWindow() = default;

    protected:
        void OnDraw() override;

    private:
        void DrawHeapInfo(const std::string& name, GfxDescriptorTableAllocator* allocator);
    };
}
