#pragma once

#include <imgui.h>
#include <string>

namespace march
{
    class GfxDevice;
    class GfxCommandContext;
    class GfxRenderTexture;

    IMGUI_IMPL_API void ImGui_ImplDX12_Init(GfxDevice* device, const std::string& shaderAssetPath);
    IMGUI_IMPL_API void ImGui_ImplDX12_Shutdown();
    IMGUI_IMPL_API void ImGui_ImplDX12_ReloadFontTexture();
    IMGUI_IMPL_API void ImGui_ImplDX12_NewFrame();
    IMGUI_IMPL_API void ImGui_ImplDX12_RenderDrawData(ImDrawData* drawData, GfxCommandContext* context, GfxRenderTexture* target);
}
