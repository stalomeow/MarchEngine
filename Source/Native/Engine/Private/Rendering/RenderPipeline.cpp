#include "pch.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Rendering/Display.h"
#include "Engine/Debug.h"
#include "Engine/Transform.h"
#include "Engine/Rendering/Gizmos.h"
#include "Engine/Misc/MathUtils.h"
#include <vector>
#include <DirectXCollision.h>

using namespace DirectX;

namespace march
{
    RenderPipeline::RenderPipeline()
    {
        m_FullScreenTriangleMesh = GfxMesh::GetGeometry(GfxMeshGeometry::FullScreenTriangle);
        m_SphereMesh = GfxMesh::GetGeometry(GfxMeshGeometry::Sphere);

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

            TextureHandle colorTarget = m_RenderGraph->Import("_CameraColorTarget", display->GetColorBuffer());
            TextureHandle depthStencilTarget = m_RenderGraph->Import("_CameraDepthStencilTarget", display->GetDepthStencilBuffer());
            TextureHandle colorTargetResolved{};

            if (display->GetEnableMSAA())
            {
                colorTargetResolved = m_RenderGraph->Import("_CameraColorTargetResolved", display->GetResolvedColorBuffer());
            }

            TestCompute();

            BufferHandle cbCamera = CreateCameraConstantBuffer("CameraConstantBuffer", camera);
            BufferHandle cbLight = CreateLightConstantBuffer();

            std::vector<TextureHandle> gBuffers{};
            ClearAndDrawObjects(cbCamera, colorTarget, depthStencilTarget, gBuffers, camera->GetEnableWireframe());

            XMFLOAT4X4 shadowMatrix{};
            TextureHandle shadowMap = DrawShadowCasters(shadowMatrix);
            TextureHandle sssMap = ScreenSpaceShadow(cbCamera, shadowMatrix, colorTarget, gBuffers, shadowMap);

            DeferredLighting(cbCamera, cbLight, colorTarget, depthStencilTarget, gBuffers, sssMap);
            DrawSkybox(cbCamera, colorTarget, depthStencilTarget);

            if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
            {
                DrawSceneViewGrid(cbCamera, colorTarget, depthStencilTarget, gridGizmoMaterial);
                Gizmos::AddRenderGraphPass(m_RenderGraph.get(), colorTarget, depthStencilTarget);
            }

            if (display->GetEnableMSAA())
            {
                ResolveMSAA(colorTarget, colorTargetResolved);
            }

            m_RenderGraph->CompileAndExecute();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("error: {}", e.what());
        }
    }

    BufferHandle RenderPipeline::CreateCameraConstantBuffer(const std::string& passName, Camera* camera)
    {
        return CreateCameraConstantBuffer(passName, camera->GetTransform()->GetPosition(), camera->GetViewMatrix(), camera->GetProjectionMatrix());
    }

    BufferHandle RenderPipeline::CreateCameraConstantBuffer(const std::string& passName, const XMFLOAT3& position, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projectionMatrix)
    {
        GfxBufferDesc desc{};
        desc.Stride = sizeof(CameraConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        static int32_t bufferId = ShaderUtils::GetIdFromString("cbCamera");
        BufferHandle buffer = m_RenderGraph->Request(bufferId, desc);

        auto builder = m_RenderGraph->AddPass(passName);

        builder.Write(buffer);
        builder.SetRenderFunc([
            buffer,
            position = position,
            viewMatrix = viewMatrix,
            projectionMatrix = projectionMatrix
        ](RenderGraphContext& context)
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

            buffer->SetData(&consts);
        });

        return buffer;
    }

    BufferHandle RenderPipeline::CreateLightConstantBuffer()
    {
        GfxBufferDesc desc{};
        desc.Stride = sizeof(LightConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        static int32 bufferId = ShaderUtils::GetIdFromString("cbLight");
        BufferHandle buffer = m_RenderGraph->Request(bufferId, desc);

        auto builder = m_RenderGraph->AddPass("LightConstantBuffer");

        builder.Write(buffer);
        builder.SetRenderFunc([buffer, this](RenderGraphContext& context)
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

            buffer->SetData(&consts);
        });

        return buffer;
    }

    void RenderPipeline::ClearAndDrawObjects(
        const BufferHandle& cbCamera,
        const TextureHandle& colorTarget,
        const TextureHandle& depthStencilTarget,
        std::vector<TextureHandle>& gBuffers,
        bool wireframe)
    {
        GfxTextureDesc gBufferDesc = colorTarget.GetDesc();

        gBufferDesc.Format = GfxTextureFormat::R8G8B8A8_UNorm;
        gBufferDesc.Flags = GfxTextureFlags::SRGB;
        gBuffers.push_back(m_RenderGraph->Request("_GBuffer0", gBufferDesc));

        gBufferDesc.Flags = GfxTextureFlags::None;
        gBuffers.push_back(m_RenderGraph->Request("_GBuffer1", gBufferDesc));
        gBuffers.push_back(m_RenderGraph->Request("_GBuffer2", gBufferDesc));

        gBufferDesc.Format = GfxTextureFormat::R32_Float;
        gBuffers.push_back(m_RenderGraph->Request("_GBuffer3", gBufferDesc));

        auto builder = m_RenderGraph->AddPass("DrawObjects");

        for (uint32_t i = 0; i < gBuffers.size(); i++)
        {
            builder.SetColorTarget(gBuffers[i], i, RenderTargetInitMode::Clear);
        }

        builder.Read(cbCamera);
        builder.SetDepthStencilTarget(depthStencilTarget, RenderTargetInitMode::Clear);
        builder.SetWireframe(wireframe);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "GBuffer");
        });
    }

    void RenderPipeline::DeferredLighting(
        const BufferHandle& cbCamera,
        const BufferHandle& cbLight,
        const TextureHandle& colorTarget,
        const TextureHandle& depthStencilTarget,
        const std::vector<TextureHandle>& gBuffers,
        const TextureHandle& screenSpaceShadowMap)
    {
        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        for (const TextureHandle& tex : gBuffers)
        {
            builder.Read(tex);
        }

        bool hasShadow = !m_Lights.empty() && !m_Renderers.empty();

        if (hasShadow)
        {
            builder.Read(screenSpaceShadowMap);
        }

        builder.Read(cbCamera);
        builder.Read(cbLight);

        builder.SetColorTarget(colorTarget, RenderTargetInitMode::Clear);
        builder.SetDepthStencilTarget(depthStencilTarget);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_FullScreenTriangleMesh, 0, m_DeferredLitMaterial.get(), 0);
        });
    }

    TextureHandle RenderPipeline::DrawShadowCasters(XMFLOAT4X4& shadowMatrix)
    {
        if (m_Lights.empty() || m_Renderers.empty())
        {
            shadowMatrix = MathUtils::Identity4x4();
            return TextureHandle();
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

        BufferHandle cbCamera = CreateCameraConstantBuffer("ShadowCameraConstantBuffer", pos, view, proj);

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

        static int32 shadowMapId = ShaderUtils::GetIdFromString("_ShadowMap");
        TextureHandle shadowMap = m_RenderGraph->Request(shadowMapId, desc);

        auto builder = m_RenderGraph->AddPass("DrawShadowCasters");

        builder.Read(cbCamera);
        builder.SetDepthStencilTarget(shadowMap, RenderTargetInitMode::Clear);
        builder.SetDepthBias(GfxSettings::ShadowDepthBias, GfxSettings::ShadowSlopeScaledDepthBias, GfxSettings::ShadowDepthBiasClamp);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "ShadowCaster");
        });

        XMMATRIX vp = XMMatrixMultiply(XMLoadFloat4x4(&view), XMLoadFloat4x4(&proj)); // DirectX 用的行向量
        XMMATRIX scale = XMMatrixScaling(0.5f, -0.5f, 1.0f);
        XMMATRIX trans = XMMatrixTranslation(0.5f, 0.5f, 0.0f);

        XMStoreFloat4x4(&shadowMatrix, XMMatrixMultiply(XMMatrixMultiply(vp, scale), trans)); // DirectX 用的行向量
        return shadowMap;
    }

    void RenderPipeline::DrawSkybox(const BufferHandle& cbCamera, const TextureHandle& colorTarget, const TextureHandle& depthStencilTarget)
    {
        auto builder = m_RenderGraph->AddPass("Skybox");

        builder.Read(cbCamera);
        builder.SetColorTarget(colorTarget);
        builder.SetDepthStencilTarget(depthStencilTarget);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_SphereMesh, 0, m_SkyboxMaterial.get(), 0);
        });
    }

    void RenderPipeline::DrawSceneViewGrid(const BufferHandle& cbCamera, const TextureHandle& colorTarget, const TextureHandle& depthStencilTarget, Material* material)
    {
        auto builder = m_RenderGraph->AddPass("SceneViewGrid");

        builder.Read(cbCamera);
        builder.SetColorTarget(colorTarget);
        builder.SetDepthStencilTarget(depthStencilTarget);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.DrawMesh(m_FullScreenTriangleMesh, 0, material, 0);
        });
    }

    void RenderPipeline::ResolveMSAA(const TextureHandle& source, const TextureHandle& destination)
    {
        auto builder = m_RenderGraph->AddPass("ResolveMSAA");

        builder.Read(source);
        builder.Write(destination);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            context.ResolveTexture(source, destination);
        });
    }

    TextureHandle RenderPipeline::ScreenSpaceShadow(
        const BufferHandle& cbCamera,
        const XMFLOAT4X4& shadowMatrix,
        const TextureHandle& colorTarget,
        const std::vector<TextureHandle>& gBuffers,
        const TextureHandle& shadowMap)
    {
        if (m_Lights.empty() || m_Renderers.empty())
        {
            return TextureHandle();
        }

        GfxTextureDesc destDesc{};
        destDesc.Format = GfxTextureFormat::R8_UNorm;
        destDesc.Flags = GfxTextureFlags::None;
        destDesc.Dimension = GfxTextureDimension::Tex2D;
        destDesc.Width = colorTarget.GetDesc().Width;
        destDesc.Height = colorTarget.GetDesc().Height;
        destDesc.DepthOrArraySize = 1;
        destDesc.MSAASamples = 1;
        destDesc.Filter = GfxTextureFilterMode::Point;
        destDesc.Wrap = GfxTextureWrapMode::Clamp;
        destDesc.MipmapBias = 0.0f;

        TextureHandle dest = m_RenderGraph->Request("_ScreenSpaceShadowMap", destDesc);

        GfxBufferDesc bufDesc{};
        bufDesc.Stride = sizeof(ShadowConstants);
        bufDesc.Count = 1;
        bufDesc.Usages = GfxBufferUsages::Constant;
        bufDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        static int32_t bufferId = ShaderUtils::GetIdFromString("cbShadow");
        BufferHandle buffer = m_RenderGraph->Request(bufferId, bufDesc);

        auto builder = m_RenderGraph->AddPass("ScreenSpaceShadow");

        for (const TextureHandle& tex : gBuffers)
        {
            builder.Read(tex);
        }

        builder.Read(cbCamera);
        builder.ReadWrite(buffer);
        builder.Read(shadowMap);

        builder.AllowPassCulling(false);
        builder.SetColorTarget(dest, RenderTargetInitMode::Discard);
        builder.SetRenderFunc([this, buffer, shadowMatrix = shadowMatrix](RenderGraphContext& context)
        {
            ShadowConstants consts{ shadowMatrix };
            buffer->SetData(&consts);
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_ScreenSpaceShadowMaterial.get(), 0);
        });

        return dest;
    }

    void RenderPipeline::TestCompute()
    {
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

        static int32_t texId = ShaderUtils::GetIdFromString("res");
        TextureHandle tex = m_RenderGraph->Request(texId, desc);

        auto builder = m_RenderGraph->AddPass("TestCompute");

        builder.AllowPassCulling(false);
        builder.EnableAsyncCompute(true);
        builder.Write(tex);
        builder.SetRenderFunc([=](RenderGraphContext& context)
        {
            std::optional<size_t> kernelIndex = m_ComputeShader->FindKernel("FillWithRed");
            context.GetCommandContext()->DispatchCompute(m_ComputeShader.get(), *kernelIndex, 4, 4, 1);
        });
    }
}
