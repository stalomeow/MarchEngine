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

#define NUM_CLUSTER_X 16
#define NUM_CLUSTER_Y 8
#define NUM_CLUSTER_Z 24
#define NUM_MAX_VISIBLE_LIGHT (NUM_CLUSTER_X * NUM_CLUSTER_Y * NUM_CLUSTER_Z * 16)

namespace march
{
    RenderPipeline::RenderPipeline()
    {
        m_DeferredLitShader.reset("Engine/Shaders/DeferredLight.shader");
        m_DeferredLitMaterial = std::make_unique<Material>();
        m_DeferredLitMaterial->SetShader(m_DeferredLitShader.get());
        m_SkyboxMaterial.reset("Assets/skybox.mat");
        m_SSAOShader.reset("Engine/Shaders/ScreenSpaceAmbientOcclusion.compute");
        m_CullLightShader.reset("Engine/Shaders/CullLight.compute");
        GenerateSSAORandomVectorMap();

        GfxBufferDesc desc1{};
        desc1.Stride = sizeof(XMINT2);
        desc1.Count = NUM_CLUSTER_X * NUM_CLUSTER_Y * NUM_CLUSTER_Z;
        desc1.Usages = GfxBufferUsages::RWStructured | GfxBufferUsages::Structured;
        desc1.Flags = GfxBufferFlags::None;
        m_ClusterPunctualLightRangesBuffer = std::make_unique<GfxBuffer>(GetGfxDevice(), "_ClusterPunctualLightRanges", desc1);

        GfxBufferDesc desc2{};
        desc2.Stride = sizeof(int32);
        desc2.Count = NUM_MAX_VISIBLE_LIGHT;
        desc2.Usages = GfxBufferUsages::RWStructured | GfxBufferUsages::Structured;
        desc2.Flags = GfxBufferFlags::None;
        m_ClusterPunctualLightIndicesBuffer = std::make_unique<GfxBuffer>(GetGfxDevice(), "_ClusterPunctualLightIndices", desc2);

        GfxBufferDesc desc3{};
        desc3.Stride = sizeof(int32);
        desc3.Count = 1;
        desc3.Usages = GfxBufferUsages::RWStructured;
        desc3.Flags = GfxBufferFlags::None;
        m_VisibleLightCounterBuffer = std::make_unique<GfxBuffer>(GetGfxDevice(), "_VisibleLightCounter", desc3);

        GfxBufferDesc desc4{};
        desc4.Stride = sizeof(int32);
        desc4.Count = NUM_CLUSTER_X * NUM_CLUSTER_Y;
        desc4.Usages = GfxBufferUsages::RWStructured;
        desc4.Flags = GfxBufferFlags::None;
        m_MaxClusterZIdsBuffer = std::make_unique<GfxBuffer>(GetGfxDevice(), "_MaxClusterZIds", desc4);

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

            if (display->GetEnableMSAA())
            {
                return;
            }

            m_Resource.Reset();
            m_Resource.ColorTarget = m_RenderGraph->ImportTexture("_CameraColorTarget", display->GetColorBuffer());
            m_Resource.DepthStencilTarget = m_RenderGraph->ImportTexture("_CameraDepthStencilTarget", display->GetDepthStencilBuffer());
            m_Resource.CbCamera = CreateCameraConstantBuffer("cbCamera", camera);

            ClearAndDrawObjects(camera->GetEnableWireframe());
            CullLights();

            XMFLOAT4X4 shadowMatrix{};
            DrawShadowCasters(shadowMatrix);

            SSAO();
            DeferredLighting(shadowMatrix);
            DrawSkybox();

            if (camera->GetEnableGizmos() && gridGizmoMaterial != nullptr)
            {
                DrawSceneViewGrid(gridGizmoMaterial);
                Gizmos::AddRenderGraphPass(m_RenderGraph.get(), m_Resource.CbCamera, m_Resource.ColorTarget, m_Resource.DepthStencilTarget);
            }

            m_RenderGraph->CompileAndExecute();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("error: {}", e.what());
        }
    }

    BufferHandle RenderPipeline::CreateCameraConstantBuffer(const std::string& name, Camera* camera)
    {
        return CreateCameraConstantBuffer(name, camera->GetTransform()->GetPosition(), camera->GetViewMatrix(), camera->GetProjectionMatrix());
    }

    BufferHandle RenderPipeline::CreateCameraConstantBuffer(const std::string& name, const XMFLOAT3& position, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projectionMatrix)
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

        GfxBufferDesc desc{};
        desc.Stride = sizeof(CameraConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        return m_RenderGraph->RequestBufferWithContent(name, desc, &consts);
    }

    void RenderPipeline::ClearAndDrawObjects(bool wireframe)
    {
        GfxTextureDesc gBufferDesc = m_Resource.ColorTarget.GetDesc();

        gBufferDesc.Format = GfxTextureFormat::R8G8B8A8_UNorm;
        gBufferDesc.Flags = GfxTextureFlags::SRGB;
        m_Resource.GBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer0", gBufferDesc));

        gBufferDesc.Flags = GfxTextureFlags::None;
        m_Resource.GBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer1", gBufferDesc));

        gBufferDesc.Format = GfxTextureFormat::R8_UNorm;
        m_Resource.GBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer2", gBufferDesc));

        gBufferDesc.Format = GfxTextureFormat::R11G11B10_Float; // HDR
        m_Resource.GBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer3", gBufferDesc));

        gBufferDesc.Format = GfxTextureFormat::R32_Float;
        m_Resource.GBuffers.push_back(m_RenderGraph->RequestTexture("_GBuffer4", gBufferDesc));

        auto builder = m_RenderGraph->AddPass("DrawObjects");

        builder.In(m_Resource.CbCamera);

        for (uint32_t i = 0; i < m_Resource.GBuffers.size(); i++)
        {
            builder.SetColorTarget(m_Resource.GBuffers[i], i, RenderTargetInitMode::Clear);
        }

        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget, RenderTargetInitMode::Clear);
        builder.SetWireframe(wireframe);

        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.CbCamera);
            context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "GBuffer");
        });
    }

    void RenderPipeline::CullLights()
    {
        static std::vector<LightData> directionalLights{};
        static std::vector<LightData> punctualLights{};

        directionalLights.clear();
        punctualLights.clear();

        for (const Light* light : m_Lights)
        {
            if (!light->GetIsActiveAndEnabled())
            {
                continue;
            }

            if (light->GetType() == LightType::Directional)
            {
                bool isFirst = directionalLights.empty();
                light->FillLightData(directionalLights.emplace_back(), isFirst);
            }
            else
            {
                light->FillLightData(punctualLights.emplace_back(), false);
            }
        }

        LightConstants consts{};
        consts.NumLights.x = static_cast<int32_t>(directionalLights.size());
        consts.NumLights.y = static_cast<int32_t>(punctualLights.size());
        consts.NumClusters.x = NUM_CLUSTER_X;
        consts.NumClusters.y = NUM_CLUSTER_Y;
        consts.NumClusters.z = NUM_CLUSTER_Z;

        GfxBufferDesc desc1{};
        desc1.Stride = sizeof(LightConstants);
        desc1.Count = 1;
        desc1.Usages = GfxBufferUsages::Constant;
        desc1.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;
        static int32 cbufferId = ShaderUtils::GetIdFromString("cbLight");
        m_Resource.CbLight = m_RenderGraph->RequestBufferWithContent(cbufferId, desc1, &consts);

        GfxBufferDesc desc2{};
        desc2.Stride = sizeof(LightData);
        desc2.Count = static_cast<uint32_t>(directionalLights.size());
        desc2.Usages = GfxBufferUsages::Structured;
        desc2.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;
        static int32 directionalLightsBufferId = ShaderUtils::GetIdFromString("_DirectionalLights");
        m_Resource.DirectionalLights = m_RenderGraph->RequestBufferWithContent(directionalLightsBufferId, desc2, directionalLights.data());

        GfxBufferDesc desc3{};
        desc3.Stride = sizeof(LightData);
        desc3.Count = static_cast<uint32_t>(punctualLights.size());
        desc3.Usages = GfxBufferUsages::Structured;
        desc3.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;
        static int32 punctualLightsBufferId = ShaderUtils::GetIdFromString("_PunctualLights");
        m_Resource.PunctualLights = m_RenderGraph->RequestBufferWithContent(punctualLightsBufferId, desc3, punctualLights.data());

        static int32 clusterPunctualLightRangesBufferId = ShaderUtils::GetIdFromString("_ClusterPunctualLightRanges");
        m_Resource.ClusterPunctualLightRanges = m_RenderGraph->ImportBuffer(clusterPunctualLightRangesBufferId, m_ClusterPunctualLightRangesBuffer.get());

        static int32 clusterPunctualLightIndicesBufferId = ShaderUtils::GetIdFromString("_ClusterPunctualLightIndices");
        m_Resource.ClusterPunctualLightIndices = m_RenderGraph->ImportBuffer(clusterPunctualLightIndicesBufferId, m_ClusterPunctualLightIndicesBuffer.get());

        static int32 visibleLightCounterBufferId = ShaderUtils::GetIdFromString("_VisibleLightCounter");
        m_Resource.VisibleLightCounter = m_RenderGraph->ImportBuffer(visibleLightCounterBufferId, m_VisibleLightCounterBuffer.get());

        static int32 maxClusterZIdsBufferId = ShaderUtils::GetIdFromString("_MaxClusterZIds");
        m_Resource.MaxClusterZIds = m_RenderGraph->ImportBuffer(maxClusterZIdsBufferId, m_MaxClusterZIdsBuffer.get());

        auto builder = m_RenderGraph->AddPass("CullLights");

        builder.EnableAsyncCompute(true);

        builder.In(m_Resource.CbCamera);
        builder.In(m_Resource.CbLight);
        builder.In(m_Resource.DirectionalLights);
        builder.In(m_Resource.PunctualLights);
        builder.Out(m_Resource.ClusterPunctualLightRanges);
        builder.Out(m_Resource.ClusterPunctualLightIndices);
        builder.Out(m_Resource.VisibleLightCounter);
        builder.Out(m_Resource.MaxClusterZIds);

        for (const TextureHandle& tex : m_Resource.GBuffers)
        {
            builder.In(tex);
        }

        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.CbCamera);
            context.SetVariable(m_Resource.CbLight);
            context.SetVariable(m_Resource.DirectionalLights);
            context.SetVariable(m_Resource.PunctualLights);
            context.SetVariable(m_Resource.ClusterPunctualLightRanges);
            context.SetVariable(m_Resource.ClusterPunctualLightIndices);
            context.SetVariable(m_Resource.VisibleLightCounter);
            context.SetVariable(m_Resource.MaxClusterZIds);

            for (const TextureHandle& tex : m_Resource.GBuffers)
            {
                context.SetVariable(tex);
            }

            // 每个 Buffer 元素对应一个线程
            std::optional<size_t> resetKernelIndex = m_CullLightShader->FindKernel("ResetMain");
            uint32_t resetCount = std::max(m_Resource.MaxClusterZIds.GetDesc().Count, 1u);
            context.DispatchComputeByThreadCount(m_CullLightShader.get(), *resetKernelIndex, resetCount, 1, 1);

            // 每个 GBuffer 像素对应一个线程
            std::optional<size_t> cullClusterKernelIndex = m_CullLightShader->FindKernel("CullClusterMain");
            uint32_t width = m_Resource.GBuffers[0].GetDesc().Width;
            uint32_t height = m_Resource.GBuffers[0].GetDesc().Height;
            context.DispatchComputeByThreadCount(m_CullLightShader.get(), *cullClusterKernelIndex, width, height, 1);

            // 每个 Cluster 对应一个线程
            std::optional<size_t> cullLightKernelIndex = m_CullLightShader->FindKernel("CullLightMain");
            context.DispatchComputeByThreadCount(m_CullLightShader.get(), *cullLightKernelIndex, NUM_CLUSTER_X, NUM_CLUSTER_Y, NUM_CLUSTER_Z);
        });
    }

    void RenderPipeline::DeferredLighting(const XMFLOAT4X4& shadowMatrix)
    {
        GfxBufferDesc bufDesc{};
        bufDesc.Stride = sizeof(ShadowConstants);
        bufDesc.Count = 1;
        bufDesc.Usages = GfxBufferUsages::Constant;
        bufDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        ShadowConstants consts{ shadowMatrix };

        static int32_t bufferId = ShaderUtils::GetIdFromString("cbShadow");
        m_Resource.CbShadow = m_RenderGraph->RequestBufferWithContent(bufferId, bufDesc, &consts);

        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        for (const TextureHandle& tex : m_Resource.GBuffers)
        {
            builder.In(tex);
        }

        builder.In(m_Resource.CbCamera);
        builder.In(m_Resource.CbLight);
        builder.In(m_Resource.DirectionalLights);
        builder.In(m_Resource.PunctualLights);
        builder.In(m_Resource.ClusterPunctualLightRanges);
        builder.In(m_Resource.ClusterPunctualLightIndices);
        builder.In(m_Resource.SSAOMap);
        builder.In(m_Resource.CbShadow);
        builder.In(m_Resource.ShadowMap);

        builder.SetColorTarget(m_Resource.ColorTarget, RenderTargetInitMode::Clear);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            for (const TextureHandle& tex : m_Resource.GBuffers)
            {
                context.SetVariable(tex);
            }

            context.SetVariable(m_Resource.CbCamera);
            context.SetVariable(m_Resource.CbLight);
            context.SetVariable(m_Resource.DirectionalLights);
            context.SetVariable(m_Resource.PunctualLights);
            context.SetVariable(m_Resource.ClusterPunctualLightRanges);
            context.SetVariable(m_Resource.ClusterPunctualLightIndices);
            context.SetVariable(m_Resource.SSAOMap);
            context.SetVariable(m_Resource.CbShadow);
            context.SetVariable(m_Resource.ShadowMap);

            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_DeferredLitMaterial.get(), 0);
        });
    }

    void RenderPipeline::DrawShadowCasters(XMFLOAT4X4& shadowMatrix)
    {
        BufferHandle cbShadowCamera{};

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

        bool drawShadow;

        if (m_Lights.empty() || m_Renderers.empty())
        {
            drawShadow = false;

            shadowMatrix = MathUtils::Identity4x4();

            desc.Width = 1;
            desc.Height = 1;
        }
        else
        {
            drawShadow = true;

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

            XMMATRIX vp = XMMatrixMultiply(XMLoadFloat4x4(&view), XMLoadFloat4x4(&proj)); // DirectX 用的行向量
            XMMATRIX scale = XMMatrixScaling(0.5f, -0.5f, 1.0f);
            XMMATRIX trans = XMMatrixTranslation(0.5f, 0.5f, 0.0f);

            XMStoreFloat4x4(&shadowMatrix, XMMatrixMultiply(XMMatrixMultiply(vp, scale), trans)); // DirectX 用的行向量

            cbShadowCamera = CreateCameraConstantBuffer("cbShadowCamera", pos, view, proj);
        }

        static int32 shadowMapId = ShaderUtils::GetIdFromString("_ShadowMap");
        m_Resource.ShadowMap = m_RenderGraph->RequestTexture(shadowMapId, desc);

        auto builder = m_RenderGraph->AddPass("DrawShadowCasters");

        if (drawShadow)
        {
            builder.In(cbShadowCamera);
        }

        builder.SetDepthStencilTarget(m_Resource.ShadowMap, RenderTargetInitMode::Clear);
        builder.SetDepthBias(GfxSettings::ShadowDepthBias, GfxSettings::ShadowSlopeScaledDepthBias, GfxSettings::ShadowDepthBiasClamp);
        builder.SetRenderFunc([this, drawShadow, cbShadowCamera](RenderGraphContext& context)
        {
            if (drawShadow)
            {
                context.SetVariable(cbShadowCamera, "cbCamera");
                context.DrawMeshRenderers(m_Renderers.size(), m_Renderers.data(), "ShadowCaster");
            }
        });
    }

    void RenderPipeline::DrawSkybox()
    {
        auto builder = m_RenderGraph->AddPass("Skybox");

        builder.In(m_Resource.CbCamera);
        builder.SetColorTarget(m_Resource.ColorTarget);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.CbCamera);
            context.DrawMesh(GfxMeshGeometry::Sphere, m_SkyboxMaterial.get(), 0);
        });
    }

    void RenderPipeline::DrawSceneViewGrid(Material* material)
    {
        auto builder = m_RenderGraph->AddPass("SceneViewGrid");

        builder.In(m_Resource.CbCamera);
        builder.SetColorTarget(m_Resource.ColorTarget);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this, material](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.CbCamera);
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, material, 0);
        });
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

    void RenderPipeline::SSAO()
    {
        SSAOConstants ssaoConsts{};
        ssaoConsts.OcclusionRadius = 0.25f;
        ssaoConsts.OcclusionFadeStart = 0.02f;
        ssaoConsts.OcclusionFadeEnd = 0.6f;
        ssaoConsts.SurfaceEpsilon = 0.01f;

        GfxBufferDesc ssaoCbDesc{};
        ssaoCbDesc.Stride = sizeof(SSAOConstants);
        ssaoCbDesc.Count = 1;
        ssaoCbDesc.Usages = GfxBufferUsages::Constant;
        ssaoCbDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        static int32 ssaoCbId = ShaderUtils::GetIdFromString("cbSSAO");
        m_Resource.CbSSAO = m_RenderGraph->RequestBufferWithContent(ssaoCbId, ssaoCbDesc, &ssaoConsts);

        GfxTextureDesc ssaoMapDesc{};
        ssaoMapDesc.Format = GfxTextureFormat::R8_UNorm;
        ssaoMapDesc.Flags = GfxTextureFlags::UnorderedAccess;
        ssaoMapDesc.Dimension = GfxTextureDimension::Tex2D;
        ssaoMapDesc.Width = static_cast<uint32_t>(ceil(m_Resource.GBuffers[0].GetDesc().Width * 0.5f));
        ssaoMapDesc.Height = static_cast<uint32_t>(ceil(m_Resource.GBuffers[0].GetDesc().Height * 0.5f));
        ssaoMapDesc.DepthOrArraySize = 1;
        ssaoMapDesc.MSAASamples = 1;
        ssaoMapDesc.Filter = GfxTextureFilterMode::Bilinear;
        ssaoMapDesc.Wrap = GfxTextureWrapMode::Clamp;
        ssaoMapDesc.MipmapBias = 0;

        static int32 ssaoMapId = ShaderUtils::GetIdFromString("_SSAOMap");
        m_Resource.SSAOMap = m_RenderGraph->RequestTexture(ssaoMapId, ssaoMapDesc);
        static int32 ssaoMapTempId = ShaderUtils::GetIdFromString("_SSAOMapTemp");
        m_Resource.SSAOMapTemp = m_RenderGraph->RequestTexture(ssaoMapTempId, ssaoMapDesc);

        static int32 randVecMapId = ShaderUtils::GetIdFromString("_RandomVecMap");
        m_Resource.SSAORandomVectorMap = m_RenderGraph->ImportTexture(randVecMapId, m_SSAORandomVectorMap.get());

        auto builder = m_RenderGraph->AddPass("ScreenSpaceAmbientOcclusion");

        builder.In(m_Resource.CbCamera);
        builder.In(m_Resource.CbSSAO);
        builder.In(m_Resource.SSAORandomVectorMap);
        builder.Out(m_Resource.SSAOMap);
        builder.Out(m_Resource.SSAOMapTemp);

        for (const TextureHandle& tex : m_Resource.GBuffers)
        {
            builder.In(tex);
        }

        builder.SetRenderFunc([this, w = ssaoMapDesc.Width, h = ssaoMapDesc.Height](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.CbCamera);
            context.SetVariable(m_Resource.CbSSAO);
            context.SetVariable(m_Resource.SSAORandomVectorMap);
            context.SetVariable(m_Resource.SSAOMap);

            for (const TextureHandle& tex : m_Resource.GBuffers)
            {
                context.SetVariable(tex);
            }

            std::optional<size_t> kernelIndex = m_SSAOShader->FindKernel("SSAOMain");
            context.DispatchComputeByThreadCount(m_SSAOShader.get(), *kernelIndex, w, h, 1);

            context.SetVariable(m_Resource.SSAOMap, "_Input");
            context.SetVariable(m_Resource.SSAOMapTemp, "_Output");

            kernelIndex = m_SSAOShader->FindKernel("HBlurMain");
            context.DispatchComputeByThreadCount(m_SSAOShader.get(), *kernelIndex, w, h, 1);

            context.SetVariable(m_Resource.SSAOMapTemp, "_Input");
            context.SetVariable(m_Resource.SSAOMap, "_Output");

            kernelIndex = m_SSAOShader->FindKernel("VBlurMain");
            context.DispatchComputeByThreadCount(m_SSAOShader.get(), *kernelIndex, w, h, 1);
        });
    }
}
