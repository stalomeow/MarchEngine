#pragma once

#include "imgui.h"

namespace march
{
    class GfxDevice;
    class GfxSwapChain;

    IMGUI_IMPL_API void ImGui_ImplDX12_Init(GfxDevice* device);
    IMGUI_IMPL_API void ImGui_ImplDX12_Shutdown();
    IMGUI_IMPL_API void ImGui_ImplDX12_ReloadFontTexture();
    IMGUI_IMPL_API void ImGui_ImplDX12_NewFrame();
    IMGUI_IMPL_API void ImGui_ImplDX12_RenderAndPresent(GfxSwapChain* mainSwapChain);
}
