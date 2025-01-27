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
#include "Engine/Graphics/GfxSettings.h"
#include "Engine/Debug.h"
#include "Engine/Transform.h"
#include "Engine/Gizmos.h"
#include "Engine/MathUtils.h"
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
        : m_CameraConstantBuffer(GetGfxDevice(), "_CameraConstantBuffer")
        , m_ShadowCameraConstantBuffer(GetGfxDevice(), "_ShadowCameraConstantBuffer")
        , m_LightConstantBuffer(GetGfxDevice(), "_LightConstantBuffer")
        , m_ShadowConstantBuffer(GetGfxDevice(), "_ShadowConstantBuffer")
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
        m_ScreenSpaceShadowShader.reset("Engine/Shaders/ScreenSpaceShadow.shader");
        m_ScreenSpaceShadowMaterial = std::make_unique<Material>(m_ScreenSpaceShadowShader.get());
        m_ComputeShader.reset("Engine/Shaders/Test.compute");

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

            TestCompute();

            if (display->GetEnableMSAA())
            {
                ImportTextures(colorTargetResolvedId, display->GetResolvedColorBuffer());
            }

            static int32 shadowMapId = Shader::GetNameId("_ShadowMap");
            XMFLOAT4X4 shadowMatrix = DrawShadowCasters(shadowMapId);

            SetCameraGlobalConstantBuffer("CameraConstantBuffer", &m_CameraConstantBuffer, camera);
            SetLightGlobalConstantBuffer(Shader::GetNameId("cbLight"));

            ClearTargets(colorTargetId, depthStencilTargetId);
            DrawObjects(colorTargetId, depthStencilTargetId, camera->GetEnableWireframe());

            static int32 screenSpaceShadowMapId = Shader::GetNameId("_ScreenSpaceShadowMap");
            ScreenSpaceShadow(shadowMatrix, colorTargetId, shadowMapId, screenSpaceShadowMapId);

            DeferredLighting(colorTargetId, depthStencilTargetId, screenSpaceShadowMapId);

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
            LOG_ERROR("error: {}", e.what());
        }
    }

    void RenderPipeline::ImportTextures(int32_t id, GfxRenderTexture* texture)
    {
        auto builder = m_RenderGraph->AddPass("Import" + Shader::GetIdName(id));
        builder.ImportTexture(id, texture);
    }

    XMFLOAT4X4 RenderPipeline::SetCameraGlobalConstantBuffer(const std::string& passName, GfxBuffer* buffer, Camera* camera)
    {
        return SetCameraGlobalConstantBuffer(passName, buffer, camera->GetTransform()->GetPosition(), camera->GetViewMatrix(), camera->GetProjectionMatrix());
    }

    XMFLOAT4X4 RenderPipeline::SetCameraGlobalConstantBuffer(const std::string& passName, GfxBuffer* buffer, const XMFLOAT3& position, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projectionMatrix)
    {
        static int32_t bufferId = Shader::GetNameId("cbCamera");

        auto builder = m_RenderGraph->AddPass(passName);

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

        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            GfxBufferDesc desc{};
            desc.Stride = sizeof(CameraConstants);
            desc.Count = 1;
            desc.Usages = GfxBufferUsages::Constant;
            desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

            buffer->SetData(desc, &consts);
            context.SetBuffer(bufferId, buffer);
        });

        return consts.ViewProjectionMatrix;
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

            GfxBufferDesc desc{};
            desc.Stride = sizeof(LightConstants);
            desc.Count = 1;
            desc.Usages = GfxBufferUsages::Constant;
            desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

            m_LightConstantBuffer.SetData(desc, &consts);
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

    void RenderPipeline::DeferredLighting(int32_t colorTargetId, int32_t depthStencilTargetId, int32 screenSpaceShadowMapId)
    {
        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        int32_t numGBuffers = 0;
        TextureHandle gBuffers[8];

        for (auto& [id, format, sRGB] : m_GBuffers)
        {
            gBuffers[numGBuffers++] = builder.ReadTexture(id);
        }

        bool hasShadow = !m_Lights.empty() && !m_Renderers.empty();
        TextureHandle shadowMap;

        if (hasShadow)
        {
            shadowMap = builder.ReadTexture(screenSpaceShadowMapId);
        }

        builder.SetColorTarget(colorTargetId);
        builder.SetDepthStencilTarget(depthStencilTargetId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            for (int32_t i = 0; i < numGBuffers; i++)
            {
                context.SetTexture(gBuffers[i].Id(), gBuffers[i].Get());
            }

            if (hasShadow)
            {
                context.SetTexture(shadowMap.Id(), shadowMap.Get());
            }

            context.DrawMesh(m_FullScreenTriangleMesh, 0, m_DeferredLitMaterial.get(), 0);
        });
    }

    XMFLOAT4X4 RenderPipeline::DrawShadowCasters(int32_t targetId)
    {
        if (m_Lights.empty() || m_Renderers.empty())
        {
            return MathUtils::Identity4x4();
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
        XMFLOAT4X4 viewProj = SetCameraGlobalConstantBuffer("ShadowCameraConstantBuffer", &m_ShadowCameraConstantBuffer, pos, view, proj);

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

        builder.CreateTransientTexture(targetId, desc);
        builder.SetDepthStencilTarget(targetId);
        builder.SetDepthBias(GfxSettings::ShadowDepthBias, GfxSettings::ShadowSlopeScaledDepthBias, GfxSettings::ShadowDepthBiasClamp);
        builder.ClearRenderTargets(GfxClearFlags::DepthStencil);

        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "ShadowCaster");
        });

        XMMATRIX vp = XMLoadFloat4x4(&viewProj);
        XMMATRIX scale = XMMatrixScaling(0.5f, -0.5f, 1.0f);
        XMMATRIX trans = XMMatrixTranslation(0.5f, 0.5f, 0.0f);

        XMFLOAT4X4 shadowMatrix{};
        XMStoreFloat4x4(&shadowMatrix, XMMatrixMultiply(XMMatrixMultiply(vp, scale), trans)); // DirectX 用的行向量
        return shadowMatrix;
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

    void RenderPipeline::ScreenSpaceShadow(const XMFLOAT4X4& shadowMatrix, int32_t cameraColorTargetId, int32_t shadowMapId, int32_t destinationId)
    {
        if (m_Lights.empty() || m_Renderers.empty())
        {
            return;
        }

        auto builder = m_RenderGraph->AddPass("ScreenSpaceShadow");

        const GfxTextureDesc& cameraTargetDesc = builder.GetTextureDesc(cameraColorTargetId);

        GfxTextureDesc desc{};
        desc.Format = GfxTextureFormat::R8_UNorm;
        desc.Flags = GfxTextureFlags::None;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = cameraTargetDesc.Width;
        desc.Height = cameraTargetDesc.Height;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Point;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0.0f;
        builder.CreateTransientTexture(destinationId, desc);

        int32_t numGBuffers = 0;
        TextureHandle gBuffers[8];

        for (auto& [id, format, sRGB] : m_GBuffers)
        {
            gBuffers[numGBuffers++] = builder.ReadTexture(id);
        }

        static int32_t bufferId = Shader::GetNameId("cbShadow");

        TextureHandle shadowMap = builder.ReadTexture(shadowMapId);

        builder.SetColorTarget(destinationId, false);
        builder.AllowPassCulling(false);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            GfxBufferDesc desc{};
            desc.Stride = sizeof(ShadowConstants);
            desc.Count = 1;
            desc.Usages = GfxBufferUsages::Constant;
            desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

            ShadowConstants consts{ shadowMatrix };
            m_ShadowConstantBuffer.SetData(desc, &consts);
            context.SetBuffer(bufferId, &m_ShadowConstantBuffer);

            for (int32_t i = 0; i < numGBuffers; i++)
            {
                context.SetTexture(gBuffers[i].Id(), gBuffers[i].Get());
            }

            context.SetTexture(shadowMapId, shadowMap.Get());
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_ScreenSpaceShadowMaterial.get(), 0);
        });
    }

    void RenderPipeline::TestCompute()
    {
        auto builder = m_RenderGraph->AddPass("TestCompute");
        builder.AllowPassCulling(false);

        static int32_t texId = Shader::GetNameId("res");
        GfxTextureDesc desc{};
        desc.Format = GfxTextureFormat::R32G32B32A32_Float;
        desc.Flags = GfxTextureFlags::UnorderedAccess;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = 4;
        desc.Height = 4;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Point;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0;

        builder.CreateTransientTexture(texId, desc);
        TextureHandle t = builder.WriteTexture(texId);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.SetTexture(texId, t.Get());

            ComputeShaderKernel* kernel = m_ComputeShader->GetKernel("FillWithRed");
            context.GetCommandContext()->DispatchCompute(m_ComputeShader.get(), kernel, {}, 4, 4, 1);
        });
    }
}
