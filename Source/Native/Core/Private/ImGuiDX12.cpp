#include "ImGuiDX12.h"
#include "GfxPipelineState.h"
#include "GfxCommand.h"
#include "GfxTexture.h"
#include "GfxDevice.h"
#include "AssetManger.h"
#include "Material.h"
#include "Shader.h"
#include "Debug.h"
#include "GfxMesh.h"
#include <directx/d3dx12.h>
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

namespace march
{
    struct ImGuiVertex : public ImDrawVert
    {
        static const GfxInputDesc& GetInputDesc()
        {
            static GfxInputDesc desc(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
                {
                    GfxInputElement(GfxSemantic::Position, DXGI_FORMAT_R32G32_FLOAT),
                    GfxInputElement(GfxSemantic::TexCoord, DXGI_FORMAT_R32G32_FLOAT),
                    GfxInputElement(GfxSemantic::Color, DXGI_FORMAT_R8G8B8A8_UNORM),
                });
            return desc;
        }
    };

    // DirectX data
    struct ImGui_ImplDX12_Data
    {
        GfxDevice* Device;
        std::string ShaderAssetPath;
        std::unique_ptr<GfxExternalTexture> FontTexture;

        asset_ptr<Shader> ShaderAsset;
        std::unique_ptr<Material> Mat;

        bool IsLoaded;

        ImGui_ImplDX12_Data()
            : Device(nullptr)
            , FontTexture(nullptr)
            , ShaderAsset(nullptr)
            , Mat(nullptr)
            , IsLoaded(false)
        {
        }
    };

    // Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
    // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
    static ImGui_ImplDX12_Data* ImGui_ImplDX12_GetBackendData()
    {
        return ImGui::GetCurrentContext() ? (ImGui_ImplDX12_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
    }

    struct Constants
    {
        XMFLOAT4X4 MVP;
    };

    // Helper structure we store in the void* RendererUserData field of each ImGuiViewport to easily retrieve our backend data.
    // Main viewport created by application will only use the Resources field.
    // Secondary viewports created by this backend will use all the fields (including Window fields),
    struct ImGui_ImplDX12_ViewportData
    {
        GfxBasicMesh<ImGuiVertex> Mesh;

        ImGui_ImplDX12_ViewportData() : Mesh(GfxSubAllocator::PersistentUpload) {}
    };

    static GfxConstantBuffer<Constants> CreateConstantBuffer(GfxDevice* device, ImDrawData* drawData)
    {
        // Ref: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx12.cpp

        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
            { 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
            { 0.0f,         0.0f,           0.5f,       0.0f },
            { (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
        };

        Constants constants{};
        memcpy(&constants.MVP.m, mvp, sizeof(mvp));

        GfxConstantBuffer<Constants> buffer{ device, GfxSubAllocator::TempUpload };
        buffer.SetData(0, &constants, sizeof(constants));
        return buffer;
    }

    // Render function
    void ImGui_ImplDX12_RenderDrawData(ImDrawData* drawData, GfxRenderTexture* intermediate, GfxRenderTexture* destination)
    {
        // Avoid rendering when minimized
        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
            return;

        ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
        ImGui_ImplDX12_ViewportData* vd = static_cast<ImGui_ImplDX12_ViewportData*>(drawData->OwnerViewport->RendererUserData);

        if (!bd->IsLoaded)
        {
            bd->IsLoaded = true;
            bd->ShaderAsset.reset(bd->ShaderAssetPath);
            bd->Mat = std::make_unique<Material>();
            bd->Mat->SetShader(bd->ShaderAsset.get());
        }

        vd->Mesh.ClearSubMeshes();

        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        uint32_t globalVtxOffset = 0;
        uint32_t globalIdxOffset = 0;

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* list = drawData->CmdLists[n];

            static_assert(sizeof(ImDrawVert) == sizeof(ImGuiVertex) && alignof(ImDrawVert) == alignof(ImGuiVertex));
            static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t) && alignof(ImDrawIdx) == alignof(uint16_t));

            vd->Mesh.AddRawVertices(static_cast<uint32_t>(list->VtxBuffer.Size), static_cast<ImGuiVertex*>(list->VtxBuffer.Data));
            vd->Mesh.AddRawIndices(static_cast<uint32_t>(list->IdxBuffer.Size), static_cast<uint16_t*>(list->IdxBuffer.Data));

            for (int cmdIndex = 0; cmdIndex < list->CmdBuffer.Size; cmdIndex++)
            {
                const ImDrawCmd* cmd = &list->CmdBuffer[cmdIndex];

                GfxSubMesh subMesh{};
                subMesh.BaseVertexLocation = static_cast<int32_t>(cmd->VtxOffset + globalVtxOffset);
                subMesh.StartIndexLocation = static_cast<uint32_t>(cmd->IdxOffset + globalIdxOffset);
                subMesh.IndexCount = static_cast<uint32_t>(cmd->ElemCount);
                vd->Mesh.AddRawSubMesh(subMesh);
            }

            globalVtxOffset += static_cast<uint32_t>(list->VtxBuffer.Size);
            globalIdxOffset += static_cast<uint32_t>(list->IdxBuffer.Size);
        }

        GfxCommandContext* context = bd->Device->RequestContext(GfxCommandType::Direct);

        context->BeginEvent("DrawImGui");
        {
            context->SetRenderTarget(intermediate);
            context->SetDefaultViewport();
            context->SetDefaultScissorRect();
            context->ClearRenderTargets(GfxClearFlags::Color);

            GfxConstantBuffer<Constants> cbuffer = CreateConstantBuffer(bd->Device, drawData);
            context->SetBuffer("cbImGui", &cbuffer);

            uint32_t subMeshIndex = 0;

            for (int n = 0; n < drawData->CmdListsCount; n++)
            {
                const ImDrawList* list = drawData->CmdLists[n];

                for (int cmdIndex = 0; cmdIndex < list->CmdBuffer.Size; cmdIndex++)
                {
                    const ImDrawCmd* cmd = &list->CmdBuffer[cmdIndex];

                    if (cmd->UserCallback != nullptr)
                    {
                        // User callback, registered via ImDrawList::AddCallback()
                        // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                        if (cmd->UserCallback == ImDrawCallback_ResetRenderState)
                        {
                            LOG_WARNING("ImDrawCallback_ResetRenderState is not supported");
                        }
                        else
                        {
                            cmd->UserCallback(list, cmd);
                        }
                    }
                    else
                    {
                        ImVec2 clipOff = drawData->DisplayPos;

                        // Project scissor/clipping rectangles into framebuffer space
                        ImVec2 clipMin(cmd->ClipRect.x - clipOff.x, cmd->ClipRect.y - clipOff.y);
                        ImVec2 clipMax(cmd->ClipRect.z - clipOff.x, cmd->ClipRect.w - clipOff.y);
                        if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                            continue;

                        context->SetScissorRect({ (LONG)clipMin.x, (LONG)clipMin.y, (LONG)clipMax.x, (LONG)clipMax.y });
                        context->SetTexture("_Texture", static_cast<GfxTexture*>(cmd->GetTexID()));
                        context->DrawMesh(vd->Mesh.GetSubMeshDesc(subMeshIndex), bd->Mat.get(), 0);
                    }

                    subMeshIndex++;
                }
            }
        }
        context->EndEvent();

        context->BeginEvent("BlitImGui");
        {
            context->SetRenderTarget(destination);
            context->SetDefaultViewport();
            context->SetDefaultScissorRect();
            context->SetTexture("_Texture", intermediate);
            context->DrawMesh(GfxMesh::GetGeometry(GfxMeshGeometry::FullScreenTriangle), 0, bd->Mat.get(), 1);
        }
        context->EndEvent();

        context->SubmitAndRelease();
    }

    void ImGui_ImplDX12_RecreateFontsTexture()
    {
        ImGuiIO& io = ImGui::GetIO();

        unsigned char* pixels = nullptr;
        int width = 0;
        int height = 0;
        int bytesPerPixel = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);

        ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
        bd->FontTexture = std::make_unique<GfxExternalTexture>(bd->Device);

        GfxTextureDesc desc{};
        desc.Format = GfxTextureFormat::R8G8B8A8_UNorm;
        desc.Flags = GfxTextureFlags::None;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = static_cast<uint32_t>(width);
        desc.Height = static_cast<uint32_t>(height);
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Bilinear;
        desc.Wrap = GfxTextureWrapMode::Repeat;
        desc.MipmapBias = 0;
        bd->FontTexture->LoadFromPixels("ImGuiFonts", desc, pixels, width * height * bytesPerPixel, 1);

        io.Fonts->SetTexID(static_cast<ImTextureID>(bd->FontTexture.get()));
    }

    void ImGui_ImplDX12_Init(GfxDevice* device, const std::string& shaderAssetPath)
    {
        ImGuiIO& io = ImGui::GetIO();
        IMGUI_CHECKVERSION();
        IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

        // Setup backend capabilities flags
        ImGui_ImplDX12_Data* bd = IM_NEW(ImGui_ImplDX12_Data)();
        io.BackendRendererUserData = (void*)bd;
        io.BackendRendererName = "imgui_impl_dx12";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

        bd->Device = device;
        bd->ShaderAssetPath = shaderAssetPath;

        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        main_viewport->RendererUserData = IM_NEW(ImGui_ImplDX12_ViewportData)();

        ImGui_ImplDX12_RecreateFontsTexture();
    }

    void ImGui_ImplDX12_Shutdown()
    {
        ImGui_ImplDX12_Data* bd = ImGui_ImplDX12_GetBackendData();
        IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
        ImGuiIO& io = ImGui::GetIO();

        // Manually delete main viewport render resources in-case we haven't initialized for viewports
        ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        if (ImGui_ImplDX12_ViewportData* vd = (ImGui_ImplDX12_ViewportData*)main_viewport->RendererUserData)
        {
            IM_DELETE(vd);
            main_viewport->RendererUserData = nullptr;
        }

        io.Fonts->SetTexID(0);
        io.BackendRendererName = nullptr;
        io.BackendRendererUserData = nullptr;
        io.BackendFlags &= ~ImGuiBackendFlags_RendererHasVtxOffset;
        IM_DELETE(bd);
    }
}
