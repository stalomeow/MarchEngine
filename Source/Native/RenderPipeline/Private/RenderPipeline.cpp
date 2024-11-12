#include "RenderPipeline.h"
#include "Debug.h"
#include "GfxTexture.h"
#include "GfxDevice.h"
#include "GfxMesh.h"
#include "GfxDescriptor.h"
#include "GfxBuffer.h"
#include "GfxCommand.h"
#include "Transform.h"
#include "Material.h"
#include "Camera.h"
#include "RenderObject.h"
#include "Display.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>
#include "RenderGraph.h"
#include "GfxUtils.h"
#include "Gizmos.h"

using namespace DirectX;

namespace march
{
    RenderPipeline::RenderPipeline()
    {
        m_FullScreenTriangleMesh = GfxMesh::GetGeometry(GfxMeshGeometry::FullScreenTriangle);

        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer0"), GfxUtils::GetShaderColorTextureFormat(DXGI_FORMAT_R8G8B8A8_UNORM));
        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer1"), DXGI_FORMAT_R8G8B8A8_UNORM);
        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer2"), DXGI_FORMAT_R8G8B8A8_UNORM);
        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer3"), DXGI_FORMAT_R32_FLOAT);
        m_DeferredLitShader.reset("Engine/Shaders/DeferredLight.shader");
        m_DeferredLitMaterial = std::make_unique<Material>();
        m_DeferredLitMaterial->SetShader(m_DeferredLitShader.get());

        m_RenderGraph = std::make_unique<RenderGraph>();
    }

    RenderPipeline::~RenderPipeline() {}

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

            DrawShadowCasters(Shader::GetNameId("ShadowMap"));

            SetCameraGlobalConstantBuffer("CameraConstantBuffer", camera);
            SetLightGlobalConstantBuffer(Shader::GetNameId("cbLight"));

            ClearTargets(colorTargetId, depthStencilTargetId);
            DrawObjects(colorTargetId, depthStencilTargetId, camera->GetEnableWireframe());
            DeferredLighting(colorTargetId, depthStencilTargetId);

            if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
            {
                DrawSceneViewGrid(colorTargetId, depthStencilTargetId, gridGizmoMaterial);
                Gizmos::AddRenderGraphPass(m_RenderGraph.get(), colorTargetId, depthStencilTargetId);
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
            LOG_ERROR("error: %s", e.what());
        }
    }

    void RenderPipeline::ImportTextures(int32_t id, GfxRenderTexture* texture)
    {
        auto builder = m_RenderGraph->AddPass("Import" + Shader::GetIdName(id));
        builder.ImportTexture(id, texture);
    }

    void RenderPipeline::SetCameraGlobalConstantBuffer(const std::string& passName, Camera* camera)
    {
        SetCameraGlobalConstantBuffer(passName, camera->GetTransform()->GetPosition(), camera->GetViewMatrix(), camera->GetProjectionMatrix());
    }

    void RenderPipeline::SetCameraGlobalConstantBuffer(const std::string& passName, const XMFLOAT3& position, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projectionMatrix)
    {
        auto builder = m_RenderGraph->AddPass(passName);

        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            XMMATRIX view = XMLoadFloat4x4(&viewMatrix);
            XMMATRIX proj = XMLoadFloat4x4(&projectionMatrix);
            XMMATRIX viewProj = XMMatrixMultiply(view, proj); // DirectX 用的行向量

            GfxDevice* device = context.GetDevice();
            auto cb = device->AllocateTransientUploadMemory(sizeof(CameraConstants), 1, GfxConstantBuffer::Alignment);

            CameraConstants& consts = *reinterpret_cast<CameraConstants*>(cb.GetMappedData(0));
            XMStoreFloat4x4(&consts.ViewMatrix, view);
            XMStoreFloat4x4(&consts.InvViewMatrix, XMMatrixInverse(nullptr, view));
            XMStoreFloat4x4(&consts.ProjectionMatrix, proj);
            XMStoreFloat4x4(&consts.InvProjectionMatrix, XMMatrixInverse(nullptr, proj));
            XMStoreFloat4x4(&consts.ViewProjectionMatrix, viewProj);
            XMStoreFloat4x4(&consts.InvViewProjectionMatrix, XMMatrixInverse(nullptr, viewProj));
            consts.CameraPositionWS = XMFLOAT4(position.x, position.y, position.z, 1.0f);

            context.SetGlobalConstantBuffer("cbCamera", cb.GetGpuVirtualAddress(0));
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

        builder.SetColorTarget(colorTargetId, false);
        builder.SetDepthStencilTarget(depthStencilTargetId, false);
        builder.ClearRenderTargets();
    }

    void RenderPipeline::DrawObjects(int32_t colorTargetId, int32_t depthStencilTargetId, bool wireframe)
    {
        auto builder = m_RenderGraph->AddPass("DrawObjects");

        GfxRenderTextureDesc desc = builder.GetTextureDesc(colorTargetId);

        for (int32_t i = 0; i < m_GBuffers.size(); i++)
        {
            auto& [id, format] = m_GBuffers[i];

            desc.Format = format;
            builder.CreateTransientTexture(id, desc);
            builder.SetColorTarget(id, i, false);
        }

        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.ClearRenderTargets(ClearFlags::Color);
        builder.SetWireframe(wireframe);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawObjects(m_RenderObjects.size(), m_RenderObjects.data(), "GBuffer");
        });
    }

    void RenderPipeline::DeferredLighting(int32_t colorTargetId, int32_t depthStencilTargetId)
    {
        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        int32_t numGBuffers = 0;
        TextureHandle gBuffers[8];

        for (auto& [id, format] : m_GBuffers)
        {
            gBuffers[numGBuffers++] = builder.ReadTexture(id, ReadFlags::PixelShader);
        }

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            for (int32_t i = 0; i < numGBuffers; i++)
            {
                context.SetTexture(gBuffers[i].Id(), gBuffers[i].Get());
            }

            context.DrawMesh(m_FullScreenTriangleMesh, m_DeferredLitMaterial.get());
        });
    }

    void RenderPipeline::DrawShadowCasters(int32_t targetId)
    {
        if (m_Lights.empty())
        {
            return;
        }

        float radius = 30.0f; // test
        XMVECTOR forward = m_Lights[0]->GetTransform()->LoadForward();
        XMVECTOR up = m_Lights[0]->GetTransform()->LoadUp();
        XMVECTOR pos = m_Lights[0]->GetTransform()->LoadPosition();

        XMFLOAT3 pos3 = {};
        XMStoreFloat3(&pos3, pos);

        XMFLOAT4X4 view = {};
        XMStoreFloat4x4(&view, XMMatrixLookToLH(pos, forward, up));

        XMFLOAT4X4 proj = {};
        XMStoreFloat4x4(&proj, XMMatrixOrthographicLH(radius * 2, radius * 2, 100, 0.01f));
        SetCameraGlobalConstantBuffer("ShadowCameraConstantBuffer", pos3, view, proj);

        auto builder = m_RenderGraph->AddPass("DrawShadowCasters");

        GfxRenderTextureDesc desc = {};
        desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        desc.Width = 2048;
        desc.Height = 2048;
        desc.SampleCount = 1;
        desc.SampleQuality = 0;

        builder.AllowPassCulling(false);
        builder.CreateTransientTexture(targetId, desc);
        builder.SetDepthStencilTarget(targetId);
        builder.ClearRenderTargets(ClearFlags::DepthStencil);

        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawObjects(m_RenderObjects.size(), m_RenderObjects.data(), "ShadowCaster");
        });
    }

    void RenderPipeline::DrawSceneViewGrid(int32_t colorTargetId, int32_t depthStencilTargetId, Material* material)
    {
        auto builder = m_RenderGraph->AddPass("SceneViewGrid");

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_FullScreenTriangleMesh, material);
        });
    }

    void RenderPipeline::PrepareTextureForImGui(int32_t id)
    {
        auto builder = m_RenderGraph->AddPass("PrepareTextureForImGui");

        builder.AllowPassCulling(false);
        builder.ReadTexture(id, ReadFlags::PixelShader);
    }
}
