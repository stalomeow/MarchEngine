#include "pch.h"
#include "Engine/Rendering/RenderPipeline.h"
#include "Engine/Rendering/Camera.h"
#include "Engine/Rendering/Display.h"
#include "Engine/Debug.h"
#include "Engine/Transform.h"
#include "Engine/Rendering/Gizmos.h"
#include "Engine/Misc/MathUtils.h"
#include <vector>
#include <random>
#include <DirectXCollision.h>
#include <DirectXPackedVector.h>

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
        m_SSAOShader.reset("Engine/Shaders/SSAO.compute");
        GenerateSSAORandomVectorMap();

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

            TextureHandle colorTarget = m_RenderGraph->ImportTexture("_CameraColorTarget", display->GetColorBuffer());
            TextureHandle depthStencilTarget = m_RenderGraph->ImportTexture("_CameraDepthStencilTarget", display->GetDepthStencilBuffer());
            TextureHandle colorTargetResolved{};

            if (display->GetEnableMSAA())
            {
                colorTargetResolved = m_RenderGraph->ImportTexture("_CameraColorTargetResolved", display->GetResolvedColorBuffer());
            }

            BufferHandle cbCamera = CreateCameraConstantBuffer("CameraConstantBuffer", camera);
            BufferHandle cbLight = CreateLightConstantBuffer();

            std::vector<TextureHandle> gBuffers{};
            ClearAndDrawObjects(cbCamera, colorTarget, depthStencilTarget, gBuffers, camera->GetEnableWireframe());

            TextureHandle ssaoMap = SSAO(cbCamera, gBuffers);

            XMFLOAT4X4 shadowMatrix{};
            TextureHandle shadowMap = DrawShadowCasters(shadowMatrix);
            TextureHandle sssMap = ScreenSpaceShadow(cbCamera, shadowMatrix, colorTarget, gBuffers, shadowMap);

            DeferredLighting(cbCamera, cbLight, colorTarget, depthStencilTarget, ssaoMap, gBuffers, sssMap);
            DrawSkybox(cbCamera, colorTarget, depthStencilTarget);

            if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
            {
                DrawSceneViewGrid(cbCamera, colorTarget, depthStencilTarget, gridGizmoMaterial);
                Gizmos::AddRenderGraphPass(m_RenderGraph.get(), cbCamera, colorTarget, depthStencilTarget);
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

        GfxBufferDesc desc{};
        desc.Stride = sizeof(CameraConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        static int32_t bufferId = ShaderUtils::GetIdFromString("cbCamera");
        return m_RenderGraph->RequestBufferWithContent(bufferId, desc, &consts);
    }

    BufferHandle RenderPipeline::CreateLightConstantBuffer()
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

        static int32 bufferId = ShaderUtils::GetIdFromString("cbLight");
        return m_RenderGraph->RequestBufferWithContent(bufferId, desc, &consts);
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
        gBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer0", gBufferDesc));

        gBufferDesc.Flags = GfxTextureFlags::None;
        gBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer1", gBufferDesc));
        gBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer2", gBufferDesc));

        gBufferDesc.Format = GfxTextureFormat::R32_Float;
        gBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer3", gBufferDesc));

        auto builder = m_RenderGraph->AddPass("DrawObjects");

        for (uint32_t i = 0; i < gBuffers.size(); i++)
        {
            builder.SetColorTarget(gBuffers[i], i, RenderTargetInitMode::Clear);
        }

        builder.In(cbCamera);
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
        const TextureHandle& ssaoMap,
        const std::vector<TextureHandle>& gBuffers,
        const TextureHandle& screenSpaceShadowMap)
    {
        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        for (const TextureHandle& tex : gBuffers)
        {
            builder.In(tex);
        }

        bool hasShadow = !m_Lights.empty() && !m_Renderers.empty();

        if (hasShadow)
        {
            builder.In(screenSpaceShadowMap);
        }

        builder.In(cbCamera);
        builder.In(cbLight);
        builder.In(ssaoMap.GetElementAs("_AOMap"));

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
        TextureHandle shadowMap = m_RenderGraph->RequestTexture(shadowMapId, desc);

        auto builder = m_RenderGraph->AddPass("DrawShadowCasters");

        builder.In(cbCamera);
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

        builder.In(cbCamera);
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

        builder.In(cbCamera);
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

        builder.In(source);
        builder.Out(destination);
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

        TextureHandle dest = m_RenderGraph->RequestTexture("_ScreenSpaceShadowMap", destDesc);

        GfxBufferDesc bufDesc{};
        bufDesc.Stride = sizeof(ShadowConstants);
        bufDesc.Count = 1;
        bufDesc.Usages = GfxBufferUsages::Constant;
        bufDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        ShadowConstants consts{ shadowMatrix };

        static int32_t bufferId = ShaderUtils::GetIdFromString("cbShadow");
        BufferHandle buffer = m_RenderGraph->RequestBufferWithContent(bufferId, bufDesc, &consts);

        auto builder = m_RenderGraph->AddPass("ScreenSpaceShadow");

        for (const TextureHandle& tex : gBuffers)
        {
            builder.In(tex);
        }

        builder.In(cbCamera);
        builder.In(shadowMap);
        builder.In(buffer);

        builder.AllowPassCulling(false);
        builder.SetColorTarget(dest, RenderTargetInitMode::Discard);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_ScreenSpaceShadowMaterial.get(), 0);
        });

        return dest;
    }

    struct SSAOConstants
    {
        float OcclusionRadius;
        float OcclusionFadeStart;
        float OcclusionFadeEnd;
        float SurfaceEpsilon;
    };

    void RenderPipeline::GenerateSSAORandomVectorMap()
    {
        m_SSAORandomVectorMap = std::make_unique<GfxExternalTexture>(GetGfxDevice());

        GfxTextureDesc desc{};
        desc.Format = GfxTextureFormat::R8G8B8A8_UNorm;
        desc.Flags = GfxTextureFlags::None;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = 256;
        desc.Height = 256;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Bilinear;
        desc.Wrap = GfxTextureWrapMode::Repeat;
        desc.MipmapBias = 0;

        std::random_device dev{};
        std::mt19937 gen(dev());  // Mersenne Twister 生成器
        std::uniform_real_distribution<float> dis(0.0, 1.0);

        std::vector<DirectX::PackedVector::XMCOLOR> data(256 * 256);

        for (size_t i = 0; i < 256; ++i)
        {
            for (size_t j = 0; j < 256; ++j)
            {
                // Random vector in [0,1].  We will decompress in shader to [-1,1].
                data[i * 256 + j] = DirectX::PackedVector::XMCOLOR(dis(gen), dis(gen), dis(gen), 0.0f);
            }
        }

        m_SSAORandomVectorMap->LoadFromPixels("_SSAORandomVectorMap", desc,
            data.data(), data.size() * sizeof(DirectX::PackedVector::XMCOLOR), 1);
    }

    TextureHandle RenderPipeline::SSAO(
        const BufferHandle& cbCamera,
        const std::vector<TextureHandle>& gBuffers)
    {
        SSAOConstants ssaoConsts{};
        ssaoConsts.OcclusionRadius = 0.25f;
        ssaoConsts.OcclusionFadeStart = 0.08f;
        ssaoConsts.OcclusionFadeEnd = 0.8f;
        ssaoConsts.SurfaceEpsilon = 0.02f;

        GfxBufferDesc ssaoCbDesc{};
        ssaoCbDesc.Stride = sizeof(SSAOConstants);
        ssaoCbDesc.Count = 1;
        ssaoCbDesc.Usages = GfxBufferUsages::Constant;
        ssaoCbDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        static int32 ssaoCbId = ShaderUtils::GetIdFromString("cbSSAO");
        BufferHandle ssaoCb = m_RenderGraph->RequestBufferWithContent(ssaoCbId, ssaoCbDesc, &ssaoConsts);

        GfxTextureDesc ssaoMapDesc{};
        ssaoMapDesc.Format = GfxTextureFormat::R8_UNorm;
        ssaoMapDesc.Flags = GfxTextureFlags::UnorderedAccess;
        ssaoMapDesc.Dimension = GfxTextureDimension::Tex2D;
        ssaoMapDesc.Width = static_cast<uint32_t>(ceil(gBuffers[0].GetDesc().Width * 0.5f));
        ssaoMapDesc.Height = static_cast<uint32_t>(ceil(gBuffers[0].GetDesc().Height * 0.5f));
        ssaoMapDesc.DepthOrArraySize = 1;
        ssaoMapDesc.MSAASamples = 1;
        ssaoMapDesc.Filter = GfxTextureFilterMode::Bilinear;
        ssaoMapDesc.Wrap = GfxTextureWrapMode::Clamp;
        ssaoMapDesc.MipmapBias = 0;

        static int32 ssaoMapId = ShaderUtils::GetIdFromString("_SSAOMap");
        TextureHandle ssaoMap = m_RenderGraph->RequestTexture(ssaoMapId, ssaoMapDesc);
        TextureHandle ssaoMap1 = m_RenderGraph->RequestTexture(ssaoMapId, ssaoMapDesc);

        static int32 randVecMapId = ShaderUtils::GetIdFromString("_RandomVecMap");
        TextureHandle randVecMap = m_RenderGraph->ImportTexture(randVecMapId, m_SSAORandomVectorMap.get());

        auto builder = m_RenderGraph->AddPass("SSAO");

        builder.AllowPassCulling(false);
        builder.EnableAsyncCompute(true);

        builder.In(cbCamera);
        builder.In(ssaoCb);
        builder.In(randVecMap);
        builder.Out(ssaoMap);

        for (const TextureHandle& tex : gBuffers)
        {
            builder.In(tex);
        }

        builder.SetRenderFunc([this, w = ssaoMapDesc.Width, h = ssaoMapDesc.Height](RenderGraphContext& context)
        {
            std::optional<size_t> kernelIndex = m_SSAOShader->FindKernel("SSAOMain");

            uint32_t groupSize[3]{};
            m_SSAOShader->GetThreadGroupSize(*kernelIndex, &groupSize[0], &groupSize[1], &groupSize[2]);

            uint32_t countX = static_cast<uint32_t>(ceil(w / static_cast<float>(groupSize[0])));
            uint32_t countY = static_cast<uint32_t>(ceil(h / static_cast<float>(groupSize[1])));
            context.DispatchCompute(m_SSAOShader.get(), *kernelIndex, countX, countY, 1);
        });

        builder = m_RenderGraph->AddPass("SSAOBlur1");

        builder.AllowPassCulling(false);
        builder.EnableAsyncCompute(true);

        builder.In(ssaoMap.GetElementAs("_Input"));
        builder.Out(ssaoMap1.GetElementAs("_Output"));

        for (const TextureHandle& tex : gBuffers)
        {
            builder.In(tex);
        }

        builder.SetRenderFunc([this, w = ssaoMapDesc.Width, h = ssaoMapDesc.Height](RenderGraphContext& context)
        {
            std::optional<size_t> kernelIndex = m_SSAOShader->FindKernel("HBlurMain");

            uint32_t groupSize[3]{};
            m_SSAOShader->GetThreadGroupSize(*kernelIndex, &groupSize[0], &groupSize[1], &groupSize[2]);

            uint32_t countX = static_cast<uint32_t>(ceil(w / static_cast<float>(groupSize[0])));
            uint32_t countY = static_cast<uint32_t>(ceil(h / static_cast<float>(groupSize[1])));
            context.DispatchCompute(m_SSAOShader.get(), *kernelIndex, countX, countY, 1);
        });

        builder = m_RenderGraph->AddPass("SSAOBlur2");

        builder.AllowPassCulling(false);
        builder.EnableAsyncCompute(true);

        builder.In(ssaoMap1.GetElementAs("_Input"));
        builder.Out(ssaoMap.GetElementAs("_Output"));

        for (const TextureHandle& tex : gBuffers)
        {
            builder.In(tex);
        }

        builder.SetRenderFunc([this, w = ssaoMapDesc.Width, h = ssaoMapDesc.Height](RenderGraphContext& context)
        {
            std::optional<size_t> kernelIndex = m_SSAOShader->FindKernel("VBlurMain");

            uint32_t groupSize[3]{};
            m_SSAOShader->GetThreadGroupSize(*kernelIndex, &groupSize[0], &groupSize[1], &groupSize[2]);

            uint32_t countX = static_cast<uint32_t>(ceil(w / static_cast<float>(groupSize[0])));
            uint32_t countY = static_cast<uint32_t>(ceil(h / static_cast<float>(groupSize[1])));
            context.DispatchCompute(m_SSAOShader.get(), *kernelIndex, countX, countY, 1);
        });

        return ssaoMap;
    }
}
