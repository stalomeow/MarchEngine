#include "pch.h"
#include "ImGuiBackend.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/AssetManger.h"
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;

namespace march
{
    class ImGuiBackendData final
    {
    public:
        ImGuiBackendData(GfxDevice* device)
            : m_Device(device)
            , m_FontTexture(nullptr)
            , m_Shader(nullptr)
            , m_Material(nullptr)
        {
            ReloadFontTexture();
        }

        void ReloadFontTexture()
        {
            ImGuiIO& io = ImGui::GetIO();

            unsigned char* pixels = nullptr;
            int width = 0;
            int height = 0;
            int bytesPerPixel = 0;
            io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);

            GfxTextureDesc desc{};
            desc.Format = GfxTextureFormat::R8G8B8A8_UNorm;
            desc.Flags = GfxTextureFlags::SRGB;
            desc.Dimension = GfxTextureDimension::Tex2D;
            desc.Width = static_cast<uint32_t>(width);
            desc.Height = static_cast<uint32_t>(height);
            desc.DepthOrArraySize = 1;
            desc.MSAASamples = 1;
            desc.Filter = GfxTextureFilterMode::Bilinear;
            desc.Wrap = GfxTextureWrapMode::Repeat;
            desc.MipmapBias = 0;

            m_FontTexture = std::make_unique<GfxExternalTexture>(m_Device);
            m_FontTexture->LoadFromPixels("ImGuiFonts", desc, pixels, width * height * bytesPerPixel, 1);
            io.Fonts->SetTexID(reinterpret_cast<ImTextureID>(m_FontTexture.get()));
        }

        Material* GetMaterial()
        {
            if (m_Material == nullptr)
            {
                m_Shader.reset("Engine/Shaders/DearImGui.shader");
                m_Material = std::make_unique<Material>(m_Shader.get());
            }

            return m_Material.get();
        }

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        GfxDevice* m_Device;

        std::unique_ptr<GfxExternalTexture> m_FontTexture;

        asset_ptr<Shader> m_Shader;
        std::unique_ptr<Material> m_Material;
    };

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

    struct ImGuiConstants
    {
        XMFLOAT4X4 MVP;
    };

    class ImGuiViewportData
    {
    public:
        explicit ImGuiViewportData(GfxDevice* device)
            : m_Device(device)
            , m_Mesh(GfxBufferFlags::Dynamic | GfxBufferFlags::Transient)
            , m_ConstantBuffer(device, "ImGuiConstants")
            , m_Intermediate(nullptr)
            , m_SwapChain(nullptr)
        {
        }

        void CreateSwapChain(HWND hWnd, uint32_t width, uint32_t height)
        {
            m_SwapChain = std::make_unique<GfxSwapChain>(m_Device, hWnd, width, height);
        }

        GfxBasicMesh<ImGuiVertex>& GetMesh()
        {
            return m_Mesh;
        }

        GfxBuffer& GetConstantBuffer()
        {
            return m_ConstantBuffer;
        }

        GfxRenderTexture* GetIntermediateTarget(GfxDevice* device, GfxRenderTexture* target)
        {
            bool needRecreate;

            if (m_Intermediate == nullptr)
            {
                needRecreate = true;
            }
            else
            {
                const GfxTextureDesc& desc1 = target->GetDesc();
                const GfxTextureDesc& desc2 = m_Intermediate->GetDesc();
                needRecreate = desc1.Width != desc2.Width || desc1.Height != desc2.Height;
            }

            if (needRecreate)
            {
                GfxTextureDesc desc{};
                desc.Format = GfxTextureFormat::R11G11B10_Float;
                desc.Flags = GfxTextureFlags::None;
                desc.Dimension = GfxTextureDimension::Tex2D;
                desc.Width = target->GetDesc().Width;
                desc.Height = target->GetDesc().Height;
                desc.DepthOrArraySize = 1;
                desc.MSAASamples = 1;
                desc.Filter = GfxTextureFilterMode::Point;
                desc.Wrap = GfxTextureWrapMode::Clamp;
                desc.MipmapBias = 0;
                m_Intermediate = std::make_unique<GfxRenderTexture>(device, "ImGuiIntermediate", desc, GfxTextureAllocStrategy::DefaultHeapCommitted);
            }

            return m_Intermediate.get();
        }

        GfxSwapChain* GetSwapChain() const { return m_SwapChain.get(); }

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        GfxDevice* m_Device;
        GfxBasicMesh<ImGuiVertex> m_Mesh;
        GfxBuffer m_ConstantBuffer;
        std::unique_ptr<GfxRenderTexture> m_Intermediate;
        std::unique_ptr<GfxSwapChain> m_SwapChain;
    };

    // Forward Declarations
    static void ImGui_ImplDX12_InitPlatformInterface();
    static void ImGui_ImplDX12_ShutdownPlatformInterface();

    // Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
    // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
    static ImGuiBackendData* ImGui_ImplDX12_GetBackendData()
    {
        return ImGui::GetCurrentContext() ? static_cast<ImGuiBackendData*>(ImGui::GetIO().BackendRendererUserData) : nullptr;
    }

    void ImGui_ImplDX12_Init(GfxDevice* device)
    {
        ImGuiIO& io = ImGui::GetIO();
        IMGUI_CHECKVERSION();
        IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

        // Setup backend capabilities flags
        ImGuiBackendData* bd = IM_NEW(ImGuiBackendData)(device);
        io.BackendRendererUserData = (void*)bd;
        io.BackendRendererName = "imgui_march_dx12";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
        io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui_ImplDX12_InitPlatformInterface();
        }

        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        mainViewport->RendererUserData = IM_NEW(ImGuiViewportData)(device);
    }

    void ImGui_ImplDX12_Shutdown()
    {
        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();
        IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
        ImGuiIO& io = ImGui::GetIO();

        // Manually delete main viewport render resources in-case we haven't initialized for viewports
        ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        if (ImGuiViewportData* vd = static_cast<ImGuiViewportData*>(mainViewport->RendererUserData))
        {
            IM_DELETE(vd);
            mainViewport->RendererUserData = nullptr;
        }

        ImGui_ImplDX12_ShutdownPlatformInterface();

        io.Fonts->SetTexID(0);
        io.BackendRendererName = nullptr;
        io.BackendRendererUserData = nullptr;
        io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasViewports);
        IM_DELETE(bd);
    }

    void ImGui_ImplDX12_ReloadFontTexture()
    {
        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();
        IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX12_Init()?");
        bd->ReloadFontTexture();
    }

    void ImGui_ImplDX12_NewFrame()
    {
        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();
        IM_ASSERT(bd != nullptr && "Context or backend not initialized! Did you call ImGui_ImplDX12_Init()?");
    }

    static void SetConstantBufferData(GfxBuffer& buffer, ImDrawData* drawData)
    {
        // Ref: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_dx12.cpp

        float L = drawData->DisplayPos.x;
        float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
        float T = drawData->DisplayPos.y;
        float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
        float mvp[4][4] =
        {
            { 2.0f / (R - L)   , 0.0f             , 0.0f, 0.0f },
            { 0.0f             , 2.0f / (T - B)   , 0.0f, 0.0f },
            { 0.0f             , 0.0f             , 0.5f, 0.0f },
            { (R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f },
        };

        GfxBufferDesc desc{};
        desc.Stride = sizeof(ImGuiConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        ImGuiConstants constants{};
        memcpy(&constants.MVP.m, mvp, sizeof(mvp));
        buffer.SetData(desc, &constants);
    }

    static void ImGui_ImplDX12_RenderDrawData(ImDrawData* drawData, GfxCommandContext* context, GfxRenderTexture* target, bool isMainViewport)
    {
        // Avoid rendering when minimized
        if (drawData->DisplaySize.x <= 0.0f || drawData->DisplaySize.y <= 0.0f)
        {
            return;
        }

        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();
        ImGuiViewportData* vd = static_cast<ImGuiViewportData*>(drawData->OwnerViewport->RendererUserData);

        GfxBasicMesh<ImGuiVertex>& mesh = vd->GetMesh();
        GfxBuffer& cbuffer = vd->GetConstantBuffer();
        GfxRenderTexture* intermediate = vd->GetIntermediateTarget(bd->GetDevice(), target);

        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        uint32_t globalVtxOffset = 0;
        uint32_t globalIdxOffset = 0;
        mesh.ClearSubMeshes();

        for (int n = 0; n < drawData->CmdListsCount; n++)
        {
            const ImDrawList* list = drawData->CmdLists[n];

            static_assert(sizeof(ImDrawVert) == sizeof(ImGuiVertex) && alignof(ImDrawVert) == alignof(ImGuiVertex));
            static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t) && alignof(ImDrawIdx) == alignof(uint16_t));

            mesh.AddRawVertices(static_cast<uint32_t>(list->VtxBuffer.Size), static_cast<ImGuiVertex*>(list->VtxBuffer.Data));
            mesh.AddRawIndices(static_cast<uint32_t>(list->IdxBuffer.Size), static_cast<uint16_t*>(list->IdxBuffer.Data));

            for (int cmdIndex = 0; cmdIndex < list->CmdBuffer.Size; cmdIndex++)
            {
                const ImDrawCmd* cmd = &list->CmdBuffer[cmdIndex];

                GfxSubMesh subMesh{};
                subMesh.BaseVertexLocation = static_cast<int32_t>(cmd->VtxOffset + globalVtxOffset);
                subMesh.StartIndexLocation = static_cast<uint32_t>(cmd->IdxOffset + globalIdxOffset);
                subMesh.IndexCount = static_cast<uint32_t>(cmd->ElemCount);
                mesh.AddRawSubMesh(subMesh);
            }

            globalVtxOffset += static_cast<uint32_t>(list->VtxBuffer.Size);
            globalIdxOffset += static_cast<uint32_t>(list->IdxBuffer.Size);
        }

        static int32_t cbufferId = ShaderUtils::GetIdFromString("ImGuiConstants");
        static int32_t textureId = ShaderUtils::GetIdFromString("_Texture");
        SetConstantBufferData(cbuffer, drawData);

        context->BeginEvent("DrawImGui");
        {
            auto setRenderState = [&]()
            {
                D3D12_VIEWPORT vp{};
                vp.TopLeftX = 0.0f;
                vp.TopLeftY = 0.0f;
                vp.Width = drawData->DisplaySize.x;
                vp.Height = drawData->DisplaySize.y;
                vp.MinDepth = 0.0f;
                vp.MaxDepth = 1.0f;

                context->SetColorTarget(intermediate);
                context->SetViewport(vp);
                context->SetDefaultScissorRect();
                context->SetBuffer(cbufferId, &cbuffer);
            };

            uint32_t subMeshIndex = 0;
            setRenderState();

            // imgui_impl_dx12.cpp 中绘制 Main Viewport 时没有 Clear，这里和它保持一致
            if (!isMainViewport && !(drawData->OwnerViewport->Flags & ImGuiViewportFlags_NoRendererClear))
            {
                context->ClearRenderTargets(GfxClearFlags::Color);
            }

            for (int n = 0; n < drawData->CmdListsCount; n++)
            {
                const ImDrawList* list = drawData->CmdLists[n];

                for (int cmdIndex = 0; cmdIndex < list->CmdBuffer.Size; cmdIndex++, subMeshIndex++)
                {
                    const ImDrawCmd* cmd = &list->CmdBuffer[cmdIndex];

                    if (cmd->UserCallback != nullptr)
                    {
                        // User callback, registered via ImDrawList::AddCallback()
                        // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
                        if (cmd->UserCallback == ImDrawCallback_ResetRenderState)
                        {
                            setRenderState();
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
                        context->SetTexture(textureId, reinterpret_cast<GfxTexture*>(cmd->GetTexID()));
                        context->DrawMesh(mesh.GetSubMeshDesc(subMeshIndex), bd->GetMaterial(), 0);
                    }
                }
            }
        }
        context->EndEvent();

        context->BeginEvent("BlitImGui");
        {
            context->SetColorTarget(target);
            context->SetDefaultViewport();
            context->SetDefaultScissorRect();
            context->SetTexture(textureId, intermediate);
            context->DrawMesh(GfxMeshGeometry::FullScreenTriangle, bd->GetMaterial(), 1);
        }
        context->EndEvent();
    }

    void ImGui_ImplDX12_RenderAndPresent(GfxSwapChain* mainSwapChain)
    {
        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();

        // Render the main window to the back buffer
        GfxCommandContext* context = bd->GetDevice()->RequestContext(GfxCommandType::Direct);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context, mainSwapChain->GetBackBuffer(), /* isMainViewport */ true);
        context->SubmitAndRelease();

        // https://github.com/ocornut/imgui/wiki/Multi-Viewports
        // Update and Render additional Platform Windows
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        mainSwapChain->Present();
    }

    //--------------------------------------------------------------------------------------------------------
    // MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
    // This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
    // If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
    //--------------------------------------------------------------------------------------------------------

    static void ImGui_ImplDX12_CreateWindow(ImGuiViewport* viewport)
    {
        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();
        ImGuiViewportData* vd = IM_NEW(ImGuiViewportData)(bd->GetDevice());

        // PlatformHandleRaw should always be a HWND, whereas PlatformHandle might be a higher-level handle (e.g. GLFWWindow*, SDL_Window*).
        // Some backends will leave PlatformHandleRaw == 0, in which case we assume PlatformHandle will contain the HWND.
        HWND hWnd = static_cast<HWND>(viewport->PlatformHandleRaw ? viewport->PlatformHandleRaw : viewport->PlatformHandle);
        IM_ASSERT(hWnd != nullptr);

        uint32_t width = static_cast<uint32_t>(viewport->Size.x);
        uint32_t height = static_cast<uint32_t>(viewport->Size.y);

        vd->CreateSwapChain(hWnd, width, height);
        viewport->RendererUserData = vd;
    }

    static void ImGui_ImplDX12_DestroyWindow(ImGuiViewport* viewport)
    {
        ImGuiViewportData* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData);

        IM_DELETE(vd);
        viewport->RendererUserData = nullptr;
    }

    static void ImGui_ImplDX12_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
    {
        ImGuiViewportData* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData);

        uint32_t width = static_cast<uint32_t>(size.x);
        uint32_t height = static_cast<uint32_t>(size.y);
        vd->GetSwapChain()->Resize(width, height);
    }

    static void ImGui_ImplDX12_RenderWindow(ImGuiViewport* viewport, void*)
    {
        ImGuiBackendData* bd = ImGui_ImplDX12_GetBackendData();
        ImGuiViewportData* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData);

        GfxCommandContext* context = bd->GetDevice()->RequestContext(GfxCommandType::Direct);
        ImGui_ImplDX12_RenderDrawData(viewport->DrawData, context, vd->GetSwapChain()->GetBackBuffer(), /* isMainViewport */ false);
        context->SubmitAndRelease();
    }

    static void ImGui_ImplDX12_SwapBuffers(ImGuiViewport* viewport, void*)
    {
        ImGuiViewportData* vd = static_cast<ImGuiViewportData*>(viewport->RendererUserData);
        vd->GetSwapChain()->Present();
    }

    static void ImGui_ImplDX12_InitPlatformInterface()
    {
        ImGuiPlatformIO& platformIO = ImGui::GetPlatformIO();
        platformIO.Renderer_CreateWindow = ImGui_ImplDX12_CreateWindow;
        platformIO.Renderer_DestroyWindow = ImGui_ImplDX12_DestroyWindow;
        platformIO.Renderer_SetWindowSize = ImGui_ImplDX12_SetWindowSize;
        platformIO.Renderer_RenderWindow = ImGui_ImplDX12_RenderWindow;
        platformIO.Renderer_SwapBuffers = ImGui_ImplDX12_SwapBuffers;
    }

    static void ImGui_ImplDX12_ShutdownPlatformInterface()
    {
        ImGui::DestroyPlatformWindows();
    }

    //-----------------------------------------------------------------------------
}
