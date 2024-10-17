#include "RenderPipeline.h"
#include "Debug.h"
#include "GfxTexture.h"
#include "GfxDevice.h"
#include "GfxMesh.h"
#include "GfxDescriptorHeap.h"
#include "GfxExcept.h"
#include "GfxBuffer.h"
#include "GfxCommandList.h"
#include "Transform.h"
#include "Material.h"
#include "Camera.h"
#include "RenderObject.h"
#include "Display.h"
#include "GfxSupportInfo.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>
#include "RenderGraph.h"

using namespace DirectX;

namespace march
{
    RenderPipeline::RenderPipeline()
    {
        m_FullScreenTriangleMesh.reset(CreateSimpleGfxMesh(GetGfxDevice()));
        m_FullScreenTriangleMesh->AddFullScreenTriangle();

        m_RenderGraph = std::make_unique<RenderGraph>();
    }

    RenderPipeline::~RenderPipeline()
    {
    }

    void RenderPipeline::Render(Camera* camera, Material* gridGizmoMaterial)
    {
        if (!camera->GetIsActiveAndEnabled())
        {
            return;
        }

        Display* display = camera->GetTargetDisplay();

        try
        {
            int32_t colorTargetId = Shader::GetNameId("_CameraColorTarget");
            int32_t colorTargetResolvedId = Shader::GetNameId("_CameraColorTargetResolved");
            int32_t depthStencilTargetId = Shader::GetNameId("_CameraDepthStencilTarget");

            ImportTextures(colorTargetId, display->GetColorBuffer());
            ImportTextures(depthStencilTargetId, display->GetDepthStencilBuffer());

            if (display->GetEnableMSAA())
            {
                ImportTextures(colorTargetResolvedId, display->GetResolvedColorBuffer());
            }

            SetCameraGlobalConstantBuffer(camera, Shader::GetNameId("cbCamera"));
            SetLightGlobalConstantBuffer(Shader::GetNameId("cbLight"));

            ClearTargets(colorTargetId, depthStencilTargetId);
            DrawObjects(colorTargetId, depthStencilTargetId, camera->GetEnableWireframe());

            if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
            {
                DrawSceneViewGrid(colorTargetId, depthStencilTargetId, gridGizmoMaterial);
            }

            if (display->GetEnableMSAA())
            {
                ResolveMSAA(colorTargetId, colorTargetResolvedId);
                PrepareTextureForImGui(colorTargetResolvedId);
            }
            else
            {
                PrepareTextureForImGui(colorTargetId);
            }

            m_RenderGraph->CompileAndExecute();
        }
        catch (std::exception& e)
        {
            DEBUG_LOG_ERROR("error: %s", e.what());
        }
    }

    void RenderPipeline::ImportTextures(int32_t id, GfxRenderTexture* texture)
    {
        auto builder = m_RenderGraph->AddPass();
        builder.ImportTexture(id, texture);
    }

    void RenderPipeline::SetCameraGlobalConstantBuffer(Camera* camera, int32_t id)
    {
        auto builder = m_RenderGraph->AddPass("CameraConstantBuffer");

        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            Display* display = camera->GetTargetDisplay();
            XMMATRIX view = camera->LoadViewMatrix();
            XMMATRIX proj = camera->LoadProjectionMatrix();
            XMMATRIX viewProj = XMMatrixMultiply(view, proj);

            GfxDevice* device = context.GetDevice();
            auto cb = device->AllocateTransientUploadMemory(sizeof(CameraConstants), 1, GfxConstantBuffer::Alignment);

            CameraConstants& consts = *reinterpret_cast<CameraConstants*>(cb.GetMappedData(0));
            XMStoreFloat4x4(&consts.ViewMatrix, view);
            XMStoreFloat4x4(&consts.InvViewMatrix, XMMatrixInverse(nullptr, view));
            XMStoreFloat4x4(&consts.ProjectionMatrix, proj);
            XMStoreFloat4x4(&consts.InvProjectionMatrix, XMMatrixInverse(nullptr, proj));
            XMStoreFloat4x4(&consts.ViewProjectionMatrix, viewProj);
            XMStoreFloat4x4(&consts.InvViewProjectionMatrix, XMMatrixInverse(nullptr, viewProj));
            XMStoreFloat4(&consts.CameraPositionWS, camera->GetTransform()->LoadPosition());

            context.SetGlobalConstantBuffer(id, cb.GetGpuVirtualAddress(0));
        });
    }

    void RenderPipeline::SetLightGlobalConstantBuffer(int32_t id)
    {
        auto builder = m_RenderGraph->AddPass("LightConstantBuffer");

        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            GfxDevice* device = context.GetDevice();
            auto cb = device->AllocateTransientUploadMemory(sizeof(LightConstants), 1, GfxConstantBuffer::Alignment);

            LightConstants& consts = *reinterpret_cast<LightConstants*>(cb.GetMappedData(0));
            consts.LightCount = static_cast<int32_t>(m_Lights.size());

            for (int i = 0; i < m_Lights.size(); i++)
            {
                if (!m_Lights[i]->GetIsActiveAndEnabled())
                {
                    continue;
                }

                m_Lights[i]->FillLightData(consts.Lights[i]);
            }

            context.SetGlobalConstantBuffer(id, cb.GetGpuVirtualAddress(0));
        });
    }

    void RenderPipeline::ResolveMSAA(int32_t id, int32_t resolvedId)
    {
        auto builder = m_RenderGraph->AddPass("ResolveMSAA");

        TextureHandle tex = builder.ReadTexture(id, ReadFlags::Resolve);
        TextureHandle resolvedTex = builder.WriteTexture(resolvedId, WriteFlags::Resolve);

        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            ID3D12GraphicsCommandList* cmd = context.GetD3D12GraphicsCommandList();
            cmd->ResolveSubresource(resolvedTex->GetD3D12Resource(), 0, tex->GetD3D12Resource(), 0, tex->GetFormat());
        });
    }

    void RenderPipeline::ClearTargets(int32_t colorTargetId, int32_t depthStencilTargetId)
    {
        auto builder = m_RenderGraph->AddPass("ClearTargets");

        builder.SetRenderTargets(colorTargetId, depthStencilTargetId);
        builder.ClearRenderTargets();
    }

    void RenderPipeline::DrawObjects(int32_t colorTargetId, int32_t depthStencilTargetId, bool wireframe)
    {
        auto builder = m_RenderGraph->AddPass("DrawObjects");

        builder.SetRenderTargets(colorTargetId, depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawObjects(m_RenderObjects.size(), m_RenderObjects.data(), wireframe);
        });
    }

    void RenderPipeline::DrawSceneViewGrid(int32_t colorTargetId, int32_t depthStencilTargetId, Material* material)
    {
        auto builder = m_RenderGraph->AddPass("SceneViewGrid");

        builder.SetRenderTargets(colorTargetId, depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_FullScreenTriangleMesh.get(), material);
        });
    }

    void RenderPipeline::PrepareTextureForImGui(int32_t id)
    {
        auto builder = m_RenderGraph->AddPass("PreserveTexture");

        builder.AllowPassCulling(false);
        builder.ReadTexture(id, ReadFlags::PixelShader);
    }
}
