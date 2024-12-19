#pragma once

#include <imgui.h>
#include <string>

namespace march
{
    class GfxDevice;
    class GfxRenderTexture;

    IMGUI_IMPL_API void ImGui_ImplDX12_Init(GfxDevice* device, const std::string& shaderAssetPath);
    IMGUI_IMPL_API void ImGui_ImplDX12_Shutdown();
    IMGUI_IMPL_API void ImGui_ImplDX12_RenderDrawData(ImDrawData* draw_data, GfxRenderTexture* intermediate, GfxRenderTexture* destination);
    IMGUI_IMPL_API void ImGui_ImplDX12_RecreateFontsTexture();
}
