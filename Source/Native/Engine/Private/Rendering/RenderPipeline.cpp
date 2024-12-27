#include "pch.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Rendering/RenderGraph.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Graphics/GfxTexture.h"
#include "Engine/Graphics/MeshRenderer.h"
#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxMesh.h"
#include "Engine/Graphics/GfxDescriptor.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/GfxCommand.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/Display.h"
#include "Engine/Graphics/GfxUtils.h"
#include "Engine/Debug.h"
#include "Engine/Transform.h"
#include "Engine/Gizmos.h"
#include <DirectXColors.h>
#include <D3Dcompiler.h>
#include <vector>
#include <array>
#include <fstream>
#include <DirectXCollision.h>

using namespace DirectX;

namespace march
{
    RenderPipeline::RenderPipeline()
    {
        m_FullScreenTriangleMesh = GfxMesh::GetGeometry(GfxMeshGeometry::FullScreenTriangle);
        m_SphereMesh = GfxMesh::GetGeometry(GfxMeshGeometry::Sphere);

        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer0"), DXGI_FORMAT_R8G8B8A8_UNORM, true);
        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer1"), DXGI_FORMAT_R8G8B8A8_UNORM, false);
        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer2"), DXGI_FORMAT_R8G8B8A8_UNORM, false);
        m_GBuffers.emplace_back(Shader::GetNameId("_GBuffer3"), DXGI_FORMAT_R32_FLOAT, false);
        m_DeferredLitShader.reset("Engine/Shaders/DeferredLight.shader");
        m_DeferredLitMaterial = std::make_unique<Material>();
        m_DeferredLitMaterial->SetShader(m_DeferredLitShader.get());
        m_SkyboxMaterial.reset("Assets/skybox.mat");

        m_RenderGraph = std::make_unique<RenderGraph>();
    }

    RenderPipeline::~RenderPipeline() {}

    void RenderPipeline::Render(Camera* camera, Material* gridGizmoMaterial)
    {
        try
        {
            if (!camera->GetIsActiveAndEnabled())
            {
                return;
            }

            Display* display = camera->GetTargetDisplay();

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

            SetCameraGlobalConstantBuffer("CameraConstantBuffer", &m_CameraConstantBuffer, camera);
            SetLightGlobalConstantBuffer(Shader::GetNameId("cbLight"));

            ClearTargets(colorTargetId, depthStencilTargetId);
            DrawObjects(colorTargetId, depthStencilTargetId, camera->GetEnableWireframe());
            DeferredLighting(colorTargetId, depthStencilTargetId);

            DrawSkybox(colorTargetId, depthStencilTargetId);

            if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
            {
                DrawSceneViewGrid(colorTargetId, depthStencilTargetId, gridGizmoMaterial);
                Gizmos::AddRenderGraphPass(m_RenderGraph.get(), colorTargetId, depthStencilTargetId);
            }

            if (display->GetEnableMSAA())
            {
                ResolveMSAA(colorTargetId, colorTargetResolvedId);
            }

            m_RenderGraph->CompileAndExecute();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("error: %s", e.what());
        }
    }

    void RenderPipeline::ImportTextures(int32_t id, GfxRenderTexture* texture)
    {
        auto builder = m_RenderGraph->AddPass("Import" + Shader::GetIdName(id));
        builder.ImportTexture(id, texture);
    }

    void RenderPipeline::SetCameraGlobalConstantBuffer(const std::string& passName, GfxConstantBuffer<CameraConstants>* buffer, Camera* camera)
    {
        SetCameraGlobalConstantBuffer(passName, buffer, camera->GetTransform()->GetPosition(), camera->GetViewMatrix(), camera->GetProjectionMatrix());
    }

    void RenderPipeline::SetCameraGlobalConstantBuffer(const std::string& passName, GfxConstantBuffer<CameraConstants>* buffer, const XMFLOAT3& position, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projectionMatrix)
    {
        static int32_t bufferId = Shader::GetNameId("cbCamera");

        auto builder = m_RenderGraph->AddPass(passName);

        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            XMMATRIX view = XMLoadFloat4x4(&viewMatrix);
            XMMATRIX proj = XMLoadFloat4x4(&projectionMatrix);
            XMMATRIX viewProj = XMMatrixMultiply(view, proj); // DirectX 用的行向量

            CameraConstants consts{};
            XMStoreFloat4x4(&consts.ViewMatrix, view);
            XMStoreFloat4x4(&consts.InvViewMatrix, XMMatrixInverse(nullptr, view));
            XMStoreFloat4x4(&consts.ProjectionMatrix, proj);
            XMStoreFloat4x4(&consts.InvProjectionMatrix, XMMatrixInverse(nullptr, proj));
            XMStoreFloat4x4(&consts.ViewProjectionMatrix, viewProj);
            XMStoreFloat4x4(&consts.InvViewProjectionMatrix, XMMatrixInverse(nullptr, viewProj));
            consts.CameraPositionWS = XMFLOAT4(position.x, position.y, position.z, 1.0f);

            *buffer = GfxConstantBuffer<CameraConstants>(context.GetDevice(), GfxSubAllocator::TempUpload);
            buffer->SetData(0, &consts, sizeof(consts));
            context.SetBuffer(bufferId, buffer);
        });
    }

    void RenderPipeline::SetLightGlobalConstantBuffer(int32_t id)
    {
        auto builder = m_RenderGraph->AddPass("LightConstantBuffer");

        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            LightConstants consts{};
            consts.LightCount = static_cast<int32_t>(m_Lights.size());

            for (int i = 0; i < m_Lights.size(); i++)
            {
                if (!m_Lights[i]->GetIsActiveAndEnabled())
                {
                    continue;
                }

                m_Lights[i]->FillLightData(consts.Lights[i]);
            }

            m_LightConstantBuffer = GfxConstantBuffer<LightConstants>(context.GetDevice(), GfxSubAllocator::TempUpload);
            m_LightConstantBuffer.SetData(0, &consts, sizeof(consts));
            context.SetBuffer(id, &m_LightConstantBuffer);
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

        GfxTextureDesc desc = builder.GetTextureDesc(colorTargetId);

        for (uint32_t i = 0; i < m_GBuffers.size(); i++)
        {
            auto& [id, format, sRGB] = m_GBuffers[i];

            desc.SetResDXGIFormat(format);
            desc.Flags = sRGB ? GfxTextureFlags::SRGB : GfxTextureFlags::None;
            builder.CreateTransientTexture(id, desc);
            builder.SetColorTarget(id, i, false);
        }

        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.ClearRenderTargets(GfxClearFlags::Color);
        builder.SetWireframe(wireframe);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "GBuffer");
        });
    }

    void RenderPipeline::DeferredLighting(int32_t colorTargetId, int32_t depthStencilTargetId)
    {
        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        int32_t numGBuffers = 0;
        TextureHandle gBuffers[8];

        for (auto& [id, format, sRGB] : m_GBuffers)
        {
            gBuffers[numGBuffers++] = builder.ReadTexture(id);
        }

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            for (int32_t i = 0; i < numGBuffers; i++)
            {
                context.SetTexture(gBuffers[i].Id(), gBuffers[i].Get());
            }

            context.DrawMesh(m_FullScreenTriangleMesh, 0, m_DeferredLitMaterial.get(), 0);
        });
    }

    void RenderPipeline::DrawShadowCasters(int32_t targetId)
    {
        if (m_Lights.empty() || m_Renderers.empty())
        {
            return;
        }

        BoundingBox aabb = m_Renderers[0]->GetBounds();

        BoundingSphere sphere = {};
        BoundingSphere::CreateFromBoundingBox(sphere, aabb);

        XMVECTOR forward = m_Lights[0]->GetTransform()->LoadForward();
        XMVECTOR up = m_Lights[0]->GetTransform()->LoadUp();

        XMVECTOR eyePos = XMLoadFloat3(&sphere.Center);
        eyePos = XMVectorSubtract(eyePos, XMVectorScale(forward, sphere.Radius + 1));

        XMFLOAT3 pos = {};
        XMStoreFloat3(&pos, eyePos);

        XMFLOAT4X4 view = {};
        XMStoreFloat4x4(&view, XMMatrixLookToLH(eyePos, forward, up));

        XMFLOAT4X4 proj = {};
        XMStoreFloat4x4(&proj, XMMatrixOrthographicLH(sphere.Radius * 2, sphere.Radius * 2, sphere.Radius * 2 + 1.0f, 1.0f));
        SetCameraGlobalConstantBuffer("ShadowCameraConstantBuffer", &m_ShadowCameraConstantBuffer, pos, view, proj);

        auto builder = m_RenderGraph->AddPass("DrawShadowCasters");

        GfxTextureDesc desc = {};
        desc.Format = GfxTextureFormat::D24_UNorm_S8_UInt;
        desc.Flags = GfxTextureFlags::None;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = 2048;
        desc.Height = 2048;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Shadow;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0.0f;

        builder.AllowPassCulling(false);
        builder.CreateTransientTexture(targetId, desc);
        builder.SetDepthStencilTarget(targetId);
        builder.ClearRenderTargets(GfxClearFlags::DepthStencil);

        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "ShadowCaster");
        });
    }

    void RenderPipeline::DrawSkybox(int32_t colorTargetId, int32_t depthStencilTargetId)
    {
        auto builder = m_RenderGraph->AddPass("Skybox");

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_SphereMesh, 0, m_SkyboxMaterial.get(), 0);
        });
    }

    void RenderPipeline::DrawSceneViewGrid(int32_t colorTargetId, int32_t depthStencilTargetId, Material* material)
    {
        auto builder = m_RenderGraph->AddPass("SceneViewGrid");

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_FullScreenTriangleMesh, 0, material, 0);
        });
    }

    void RenderPipeline::ResolveMSAA(int32_t sourceId, int32_t destinationId)
    {
        auto builder = m_RenderGraph->AddPass("ResolveMSAA");

        TextureHandle source = builder.ReadTexture(sourceId);
        TextureHandle destination = builder.WriteTexture(destinationId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.ResolveTexture(source.Get(), destination.Get());
        });
    }
}
