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
        m_RenderGraph = std::make_unique<RenderGraph>();

        m_DeferredLitShader.reset("Engine/Shaders/DeferredLight.shader");
        m_DeferredLitMaterial = std::make_unique<Material>(m_DeferredLitShader.get());

        m_SceneViewGridShader.reset("Engine/Shaders/SceneViewGrid.shader");
        m_SceneViewGridMaterial = std::make_unique<Material>(m_SceneViewGridShader.get());

        m_CameraMotionVectorShader.reset("Engine/Shaders/CameraMotionVector.shader");
        m_CameraMotionVectorMaterial = std::make_unique<Material>(m_CameraMotionVectorShader.get());

        m_SSAOShader.reset("Engine/Shaders/ScreenSpaceAmbientOcclusion.compute");
        m_CullLightShader.reset("Engine/Shaders/CullLight.compute");
        m_DiffuseIrradianceShader.reset("Engine/Shaders/DiffuseIrradiance.compute");
        m_SpecularIBLShader.reset("Engine/Shaders/SpecularIBL.compute");
        m_TAAShader.reset("Engine/Shaders/TemporalAntialiasing.compute");
        m_PostprocessingShader.reset("Engine/Shaders/Postprocessing.compute");
        m_HiZShader.reset("Engine/Shaders/HierarchicalZ.compute");

        CreateLightResources();
        GenerateSSAORandomVectorMap();
        CreateEnvLightResources();
    }

    void RenderPipeline::AddMeshRenderer(MeshRenderer* obj)
    {
        m_MeshRenderers.push_back(obj);
    }

    void RenderPipeline::RemoveMeshRenderer(MeshRenderer* obj)
    {
        auto it = std::find(m_MeshRenderers.begin(), m_MeshRenderers.end(), obj);

        if (it != m_MeshRenderers.end())
        {
            m_MeshRenderers.erase(it);
        }
    }

    void RenderPipeline::AddLight(Light* light)
    {
        m_Lights.push_back(light);
    }

    void RenderPipeline::RemoveLight(Light* light)
    {
        auto it = std::find(m_Lights.begin(), m_Lights.end(), light);

        if (it != m_Lights.end())
        {
            m_Lights.erase(it);
        }
    }

    void RenderPipeline::SetSkyboxMaterial(Material* material)
    {
        m_SkyboxMaterial = material;
    }

    void RenderPipeline::PrepareFrameData()
    {
        for (MeshRenderer* renderer : m_MeshRenderers)
        {
            renderer->PrepareFrameData();
        }

        for (Camera* camera : Camera::GetAllCameras())
        {
            camera->PrepareFrameData();
        }
    }

    void RenderPipeline::Render()
    {
        for (Camera* camera : Camera::GetAllCameras())
        {
            RenderSingleCamera(camera);
        }
    }

    void RenderPipeline::RenderSingleCamera(Camera* camera)
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
                LOG_WARNING("MSAA is not supported in current render pipeline implementation (because I am lazy).");
                return;
            }

            m_Resource.Reset();
            m_Resource.ColorTarget = m_RenderGraph->ImportTexture("_CameraColorTarget", display->GetColorBuffer());
            m_Resource.DepthStencilTarget = m_RenderGraph->ImportTexture("_CameraDepthStencilTarget", display->GetDepthStencilBuffer());
            m_Resource.HistoryColorTexture = m_RenderGraph->ImportTexture("_HistoryColorTexture", display->GetHistoryColorBuffer());
            m_Resource.CbCamera = CreateCameraConstantBuffer("cbCamera", camera);
            m_Resource.EnvDiffuseSH9Coefs = m_RenderGraph->ImportBuffer("_EnvDiffuseSH9Coefs", m_EnvDiffuseSH9Coefs.get());
            m_Resource.EnvSpecularRadianceMap = m_RenderGraph->ImportTexture("_EnvSpecularRadianceMap", m_EnvSpecularRadianceMap.get());
            m_Resource.EnvSpecularBRDFLUT = m_RenderGraph->ImportTexture("_EnvSpecularBRDFLUT", m_EnvSpecularBRDFLUT.get());
            m_Resource.RequestGBuffers(m_RenderGraph.get(), display->GetPixelWidth(), display->GetPixelHeight());

            ClearAndDrawGBuffers(camera->GetEnableWireframe());

            HiZ();
            CullLights();
            SSAO();

            DrawMotionVector();

            XMFLOAT4X4 shadowMatrix{};
            float depth2RadialScale = 0;
            DrawShadowCasters(shadowMatrix, depth2RadialScale);
            DeferredLighting(shadowMatrix, depth2RadialScale);
            DrawSkybox();

            if (camera->GetEnableGizmos())
            {
                DrawSceneViewGrid();
                DrawGizmos();
            }

            TAA();
            Postprocessing();

            m_RenderGraph->CompileAndExecute();
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("RenderPipelineError: {}", e.what());
        }
    }

    struct CameraConstants
    {
        XMFLOAT4X4 ViewMatrix;
        XMFLOAT4X4 ProjectionMatrix;
        XMFLOAT4X4 ViewProjectionMatrix;
        XMFLOAT4X4 InvViewMatrix;
        XMFLOAT4X4 InvProjectionMatrix;
        XMFLOAT4X4 InvViewProjectionMatrix;
        XMFLOAT4X4 NonJitteredViewProjectionMatrix;
        XMFLOAT4X4 PrevNonJitteredViewProjectionMatrix;
        XMFLOAT4 TAAParams; // x: TAAFrameIndex
    };

    BufferHandle RenderPipeline::CreateCameraConstantBuffer(const std::string& name, Camera* camera)
    {
        CameraConstants consts{};
        consts.ViewMatrix = camera->GetViewMatrix();
        consts.ProjectionMatrix = camera->GetProjectionMatrix();
        consts.ViewProjectionMatrix = camera->GetViewProjectionMatrix();
        XMStoreFloat4x4(&consts.InvViewMatrix, XMMatrixInverse(nullptr, XMLoadFloat4x4(&consts.ViewMatrix)));
        XMStoreFloat4x4(&consts.InvProjectionMatrix, XMMatrixInverse(nullptr, XMLoadFloat4x4(&consts.ProjectionMatrix)));
        XMStoreFloat4x4(&consts.InvViewProjectionMatrix, XMMatrixInverse(nullptr, XMLoadFloat4x4(&consts.ViewProjectionMatrix)));
        consts.NonJitteredViewProjectionMatrix = camera->GetNonJitteredViewProjectionMatrix();
        consts.PrevNonJitteredViewProjectionMatrix = camera->GetPrevNonJitteredViewProjectionMatrix();
        consts.TAAParams.x = static_cast<float>(camera->GetTAAFrameIndex());

        GfxBufferDesc desc{};
        desc.Stride = sizeof(CameraConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        return m_RenderGraph->RequestBufferWithContent(name, desc, &consts);
    }

    BufferHandle RenderPipeline::CreateCameraConstantBuffer(const std::string& name, const XMFLOAT4X4& viewMatrix, const XMFLOAT4X4& projectionMatrix)
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

        // 不设置 TAA 的参数

        GfxBufferDesc desc{};
        desc.Stride = sizeof(CameraConstants);
        desc.Count = 1;
        desc.Usages = GfxBufferUsages::Constant;
        desc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        return m_RenderGraph->RequestBufferWithContent(name, desc, &consts);
    }

    void RenderPipeline::ClearAndDrawGBuffers(bool wireframe)
    {
        auto builder = m_RenderGraph->AddPass("DrawGBuffer");

        builder.In(m_Resource.CbCamera);

        for (uint32_t i = 0; i < m_Resource.NumGBuffers; i++)
        {
            builder.SetColorTarget(m_Resource.GBuffers[i], i, RenderTargetInitMode::Clear);
        }

        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget, RenderTargetInitMode::Clear);

        builder.SetWireframe(wireframe);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.DrawMeshRenderers(m_MeshRenderers.size(), m_MeshRenderers.data(), "GBuffer");
        });
    }

    void RenderPipeline::CreateLightResources()
    {
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
    }

    struct LightConstants
    {
        XMINT4 NumLights;   // x: numDirectionalLights, y: numPunctualLights
        XMINT4 NumClusters; // xyz: numClusters
    };

    void RenderPipeline::CullLights()
    {
        static std::vector<LightData> directionalLights{};
        static std::vector<LightData> punctualLights{};

        directionalLights.clear();
        punctualLights.clear();
        bool hasDirectionalLightShadow = false;

        for (const Light* light : m_Lights)
        {
            if (!light->GetIsActiveAndEnabled())
            {
                continue;
            }

            if (light->GetType() == LightType::Directional)
            {
                bool castShadow = false;

                if (!hasDirectionalLightShadow && light->GetIsCastingShadow())
                {
                    hasDirectionalLightShadow = true;
                    castShadow = true;
                }

                light->FillLightData(directionalLights.emplace_back(), castShadow);
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
        builder.In(m_Resource.PunctualLights);
        builder.In(m_Resource.HiZTexture);
        builder.Out(m_Resource.ClusterPunctualLightRanges);
        builder.Out(m_Resource.ClusterPunctualLightIndices);
        builder.Out(m_Resource.VisibleLightCounter);
        builder.Out(m_Resource.MaxClusterZIds);

        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            // 每个 Buffer 元素对应一个线程
            uint32_t resetCount = std::max(m_Resource.MaxClusterZIds.GetDesc().Count, 1u);
            context.DispatchComputeByThreadCount(m_CullLightShader.get(), "ResetMain", resetCount, 1, 1);

            // 每个像素对应一个线程
            uint32_t width = m_Resource.HiZTexture.GetDesc().Width;
            uint32_t height = m_Resource.HiZTexture.GetDesc().Height;
            context.DispatchComputeByThreadCount(m_CullLightShader.get(), "CullClusterMain", width, height, 1);

            // 每个 Cluster 对应一个线程
            context.DispatchComputeByThreadCount(m_CullLightShader.get(), "CullLightMain", NUM_CLUSTER_X, NUM_CLUSTER_Y, NUM_CLUSTER_Z);
        });
    }

    struct ShadowConstants
    {
        XMFLOAT4X4 ShadowMatrix;
        XMFLOAT4 ShadowParams; // x: depth2RadialScale
    };

    void RenderPipeline::DeferredLighting(const XMFLOAT4X4& shadowMatrix, float depth2RadialScale)
    {
        GfxBufferDesc bufDesc{};
        bufDesc.Stride = sizeof(ShadowConstants);
        bufDesc.Count = 1;
        bufDesc.Usages = GfxBufferUsages::Constant;
        bufDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

        ShadowConstants consts{};
        consts.ShadowMatrix = shadowMatrix;
        consts.ShadowParams.x = depth2RadialScale;

        static int32_t bufferId = ShaderUtils::GetIdFromString("cbShadow");
        m_Resource.CbShadow = m_RenderGraph->RequestBufferWithContent(bufferId, bufDesc, &consts);

        auto builder = m_RenderGraph->AddPass("DeferredLighting");

        builder.In(m_Resource.GetGBuffers(GBufferElements::All));
        builder.In(m_Resource.CbCamera);
        builder.In(m_Resource.CbLight);
        builder.In(m_Resource.DirectionalLights);
        builder.In(m_Resource.PunctualLights);
        builder.In(m_Resource.ClusterPunctualLightRanges);
        builder.In(m_Resource.ClusterPunctualLightIndices);
        builder.In(m_Resource.SSAOMap);
        builder.In(m_Resource.CbShadow);
        builder.In(m_Resource.ShadowMap);
        builder.In(m_Resource.EnvDiffuseSH9Coefs);
        builder.In(m_Resource.EnvSpecularRadianceMap);
        builder.In(m_Resource.EnvSpecularBRDFLUT);
        builder.In(m_Resource.HiZTexture);

        builder.SetColorTarget(m_Resource.ColorTarget, RenderTargetInitMode::Clear);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_DeferredLitMaterial.get(), 0);
        });
    }

    void RenderPipeline::DrawShadowCasters(XMFLOAT4X4& shadowMatrix, float& depth2RadialScale)
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

        bool drawShadow = false;
        size_t lightIndex = 0;

        if (!m_Lights.empty() && !m_MeshRenderers.empty())
        {
            for (size_t i = 0; i < m_Lights.size(); i++)
            {
                const Light* light = m_Lights[i];

                if (light->GetIsActiveAndEnabled() && light->GetType() == LightType::Directional && light->GetIsCastingShadow())
                {
                    drawShadow = true;
                    lightIndex = i;
                    break;
                }
            }
        }

        if (!drawShadow)
        {
            shadowMatrix = MathUtils::Identity4x4();
            depth2RadialScale = 0;

            desc.Width = 1;
            desc.Height = 1;
        }
        else
        {
            BoundingBox aabb{};

            for (size_t i = 0; i < m_MeshRenderers.size(); i++)
            {
                if (i == 0)
                {
                    aabb = m_MeshRenderers[i]->GetBounds();
                }
                else
                {
                    BoundingBox temp{};
                    BoundingBox::CreateMerged(temp, aabb, m_MeshRenderers[i]->GetBounds());
                    aabb = temp;
                }
            }

            BoundingSphere sphere = {};
            BoundingSphere::CreateFromBoundingBox(sphere, aabb);

            XMVECTOR forward = m_Lights[lightIndex]->GetTransform()->LoadForward();
            XMVECTOR up = m_Lights[lightIndex]->GetTransform()->LoadUp();

            XMVECTOR eyePos = XMLoadFloat3(&sphere.Center);
            eyePos = XMVectorSubtract(eyePos, XMVectorScale(forward, sphere.Radius + 1));

            XMFLOAT4X4 view = {};
            XMStoreFloat4x4(&view, XMMatrixLookToLH(eyePos, forward, up));

            XMFLOAT4X4 proj = {};
            XMStoreFloat4x4(&proj, XMMatrixOrthographicLH(sphere.Radius * 2, sphere.Radius * 2, sphere.Radius * 2 + 1.0f, 1.0f));

            XMMATRIX vp = XMMatrixMultiply(XMLoadFloat4x4(&view), XMLoadFloat4x4(&proj)); // DirectX 用的行向量
            XMMATRIX scale = XMMatrixScaling(0.5f, -0.5f, 1.0f);
            XMMATRIX trans = XMMatrixTranslation(0.5f, 0.5f, 0.0f);

            XMStoreFloat4x4(&shadowMatrix, XMMatrixMultiply(XMMatrixMultiply(vp, scale), trans)); // DirectX 用的行向量

            float s = std::tanf(XMConvertToRadians(0.5f * m_Lights[lightIndex]->GetAngularDiameter()));
            depth2RadialScale = s * std::abs(proj._11 / proj._33);

            cbShadowCamera = CreateCameraConstantBuffer("cbShadowCamera", view, proj);
        }

        static int32 shadowMapId = ShaderUtils::GetIdFromString("_ShadowMap");
        m_Resource.ShadowMap = m_RenderGraph->RequestTexture(shadowMapId, desc);

        auto builder = m_RenderGraph->AddPass("DrawShadowCasters");

        if (drawShadow)
        {
            builder.In(cbShadowCamera);
            builder.SetDepthBias(
                m_Lights[lightIndex]->GetShadowDepthBias(),
                m_Lights[lightIndex]->GetShadowSlopeScaledDepthBias(),
                m_Lights[lightIndex]->GetShadowDepthBiasClamp());
        }

        builder.UseDefaultVariables(false);

        builder.SetDepthStencilTarget(m_Resource.ShadowMap, RenderTargetInitMode::Clear);
        builder.SetRenderFunc([this, drawShadow, cbShadowCamera](RenderGraphContext& context)
        {
            if (drawShadow)
            {
                context.SetVariable(cbShadowCamera, "cbCamera");
                context.DrawMeshRenderers(m_MeshRenderers.size(), m_MeshRenderers.data(), "ShadowCaster");
            }
        });
    }

    void RenderPipeline::DrawSkybox()
    {
        if (m_SkyboxMaterial == nullptr)
        {
            return;
        }

        auto builder = m_RenderGraph->AddPass("Skybox");

        builder.In(m_Resource.CbCamera);
        builder.SetColorTarget(m_Resource.ColorTarget);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.DrawMesh(GfxMeshGeometry::Sphere, m_SkyboxMaterial, 0);
        });
    }

    void RenderPipeline::DrawGizmos()
    {
        if (!Gizmos::CanRender())
        {
            return;
        }

        auto builder = m_RenderGraph->AddPass("Gizmos");

        builder.In(m_Resource.CbCamera);
        builder.SetColorTarget(m_Resource.ColorTarget);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc(Gizmos::Render);
    }

    void RenderPipeline::DrawSceneViewGrid()
    {
        auto builder = m_RenderGraph->AddPass("SceneViewGrid");

        builder.In(m_Resource.CbCamera);
        builder.SetColorTarget(m_Resource.ColorTarget);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_SceneViewGridMaterial.get(), 0);
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

        std::vector<PackedVector::XMCOLOR> data(256 * 256);

        for (size_t i = 0; i < 256; ++i)
        {
            for (size_t j = 0; j < 256; ++j)
            {
                // Random vector in [0,1].  We will decompress in shader to [-1,1].
                data[i * 256 + j] = PackedVector::XMCOLOR(dis(gen), dis(gen), dis(gen), 0.0f);
            }
        }

        m_SSAORandomVectorMap->LoadFromPixels("_SSAORandomVectorMap", desc,
            data.data(), data.size() * sizeof(PackedVector::XMCOLOR), 1);
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
        ssaoMapDesc.Width = static_cast<uint32_t>(ceil(m_Resource.ColorTarget.GetDesc().Width * 0.5f));
        ssaoMapDesc.Height = static_cast<uint32_t>(ceil(m_Resource.ColorTarget.GetDesc().Height * 0.5f));
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

        builder.EnableAsyncCompute(true);

        builder.In(m_Resource.CbCamera);
        builder.In(m_Resource.CbSSAO);
        builder.In(m_Resource.GetGBuffers(GBufferElements::Normal));
        builder.In(m_Resource.SSAORandomVectorMap);
        builder.In(m_Resource.HiZTexture);
        builder.Out(m_Resource.SSAOMap);
        builder.Out(m_Resource.SSAOMapTemp);

        builder.SetRenderFunc([this, w = ssaoMapDesc.Width, h = ssaoMapDesc.Height](RenderGraphContext& context)
        {
            context.DispatchComputeByThreadCount(m_SSAOShader.get(), "SSAOMain", w, h, 1);

            context.SetVariable(m_Resource.SSAOMap, "_Input");
            context.SetVariable(m_Resource.SSAOMapTemp, "_Output");
            context.DispatchComputeByThreadCount(m_SSAOShader.get(), "HBlurMain", w, h, 1);

            context.SetVariable(m_Resource.SSAOMapTemp, "_Input");
            context.SetVariable(m_Resource.SSAOMap, "_Output");
            context.DispatchComputeByThreadCount(m_SSAOShader.get(), "VBlurMain", w, h, 1);
        });
    }

    void RenderPipeline::CreateEnvLightResources()
    {
        GfxBufferDesc sh9Desc{};
        sh9Desc.Stride = sizeof(XMFLOAT3);
        sh9Desc.Count = 9;
        sh9Desc.Usages = GfxBufferUsages::Structured | GfxBufferUsages::RWStructured;
        sh9Desc.Flags = GfxBufferFlags::None;
        m_EnvDiffuseSH9Coefs = std::make_unique<GfxBuffer>(GetGfxDevice(), "_EnvDiffuseSH9Coefs", sh9Desc);

        GfxTextureDesc desc1{};
        desc1.Format = GfxTextureFormat::R11G11B10_Float;
        desc1.Flags = GfxTextureFlags::UnorderedAccess | GfxTextureFlags::Mipmaps;
        desc1.Dimension = GfxTextureDimension::Cube;
        desc1.Width = 128;
        desc1.Height = 128;
        desc1.DepthOrArraySize = 1;
        desc1.MSAASamples = 1;
        desc1.Filter = GfxTextureFilterMode::Trilinear; // 在多个 roughness 之间插值
        desc1.Wrap = GfxTextureWrapMode::Clamp;
        desc1.MipmapBias = 0;
        m_EnvSpecularRadianceMap = std::make_unique<GfxRenderTexture>(GetGfxDevice(), "_EnvSpecularRadianceMap", desc1, GfxTextureAllocStrategy::DefaultHeapCommitted);

        // GenerateEnvSpecularBRDFLUT();
        // 现在不需要每次启动时都重新生成，可以直接加载
        m_EnvSpecularBRDFLUT.reset("Engine/Resources/Textures/EnvSpecularBRDFLUT.dds");
    }

    void RenderPipeline::GenerateEnvSpecularBRDFLUT()
    {
        //GfxCommandContext* cmd = GetGfxDevice()->RequestContext(GfxCommandType::Direct);

        //cmd->BeginEvent("SpecularBRDFLUT");
        //{
        //    GfxTextureDesc desc{};
        //    desc.Format = GfxTextureFormat::R16G16_Float;
        //    desc.Flags = GfxTextureFlags::UnorderedAccess;
        //    desc.Dimension = GfxTextureDimension::Tex2D;
        //    desc.Width = 512;
        //    desc.Height = 512;
        //    desc.DepthOrArraySize = 1;
        //    desc.MSAASamples = 1;
        //    desc.Filter = GfxTextureFilterMode::Bilinear;
        //    desc.Wrap = GfxTextureWrapMode::Clamp;
        //    desc.MipmapBias = 0;

        //    m_EnvSpecularBRDFLUT = std::make_unique<GfxRenderTexture>(GetGfxDevice(), "_EnvSpecularBRDFLUT", desc, GfxTextureAllocStrategy::DefaultHeapCommitted);

        //    cmd->SetTexture("_BRDFLUT", m_EnvSpecularBRDFLUT.get());
        //    cmd->DispatchComputeByThreadCount(m_SpecularIBLShader.get(), "BRDFMain", desc.Width, desc.Height, 1);
        //}
        //cmd->EndEvent();

        //cmd->SubmitAndRelease();
    }

    struct DiffuseIrradianceSH9Consts
    {
        float IntensityMultiplier;
    };

    struct SpecularIBLPrefilterConsts
    {
        XMFLOAT3 Params; // x: roughness, y: face, z: intensityMultiplier
    };

    void RenderPipeline::BakeEnvLight(GfxTexture* radianceMap, float diffuseIntensityMultiplier, float specularIntensityMultiplier)
    {
        GfxCommandContext* cmd = GetGfxDevice()->RequestContext(GfxCommandType::Direct);

        cmd->BeginEvent("BakeEnvLight");
        {
            // Diffuse Irradiance
            {
                cmd->SetTexture("_RadianceMap", radianceMap);
                cmd->SetBuffer("_SH9Coefs", m_EnvDiffuseSH9Coefs.get());

                GfxBufferDesc cbDesc{};
                cbDesc.Stride = sizeof(DiffuseIrradianceSH9Consts);
                cbDesc.Count = 1;
                cbDesc.Usages = GfxBufferUsages::Constant;
                cbDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

                DiffuseIrradianceSH9Consts consts{ std::clamp(diffuseIntensityMultiplier, 0.0f, 1.0f)};
                GfxBuffer cb{ GetGfxDevice(), "cbSH9", cbDesc };
                cb.SetData(&consts);

                cmd->SetBuffer("cbSH9", &cb);
                cmd->DispatchCompute(m_DiffuseIrradianceShader.get(), "CalcSH9Main", 1, 1, 1);
            }

            cmd->UnsetTexturesAndBuffers();

            // Specular IBL
            {
                cmd->SetTexture("_RadianceMap", radianceMap);

                const GfxTextureDesc& outputDesc = m_EnvSpecularRadianceMap->GetDesc();

                GfxBufferDesc cbDesc{};
                cbDesc.Stride = sizeof(SpecularIBLPrefilterConsts);
                cbDesc.Count = 1;
                cbDesc.Usages = GfxBufferUsages::Constant;
                cbDesc.Flags = GfxBufferFlags::Dynamic | GfxBufferFlags::Transient;

                SpecularIBLPrefilterConsts consts{};
                consts.Params.z = std::clamp(specularIntensityMultiplier, 0.0f, 1.0f);

                // 最后一级 mip 每个面只有一个像素，忽略它
                for (uint32_t mip = 0; mip < m_EnvSpecularRadianceMap->GetMipLevels() - 1; mip++)
                {
                    float roughness = static_cast<float>(mip) / (m_EnvSpecularRadianceMap->GetMipLevels() - 2);
                    cmd->SetTexture("_PrefilteredMap", m_EnvSpecularRadianceMap.get(), GfxTextureElement::Default, mip);

                    consts.Params.x = roughness;

                    for (uint32_t face = 0; face < 6; face++)
                    {
                        consts.Params.y = static_cast<float>(face);

                        GfxBuffer cb{ GetGfxDevice(), "cbPrefilter", cbDesc };
                        cb.SetData(&consts);

                        cmd->SetBuffer("cbPrefilter", &cb);
                        cmd->DispatchComputeByThreadCount(m_SpecularIBLShader.get(), "PrefilterMain", outputDesc.Width, outputDesc.Height, 1);
                    }
                }
            }
        }
        cmd->EndEvent();

        cmd->SubmitAndRelease();
    }

    void RenderPipeline::DrawMotionVector()
    {
        GfxTextureDesc desc{};
        desc.Format = GfxTextureFormat::R16G16_Float;
        desc.Flags = GfxTextureFlags::None;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = m_Resource.ColorTarget.GetDesc().Width;
        desc.Height = m_Resource.ColorTarget.GetDesc().Height;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Point;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0;

        static int32 mvId = ShaderUtils::GetIdFromString("_MotionVectorTexture");
        m_Resource.MotionVectorTexture = m_RenderGraph->RequestTexture(mvId, desc);

        auto builder = m_RenderGraph->AddPass("MotionVector");

        builder.In(m_Resource.CbCamera);
        builder.SetColorTarget(m_Resource.MotionVectorTexture, RenderTargetInitMode::Clear);
        builder.SetDepthStencilTarget(m_Resource.DepthStencilTarget);
        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.DrawMesh(GfxMeshGeometry::FullScreenTriangle, m_CameraMotionVectorMaterial.get(), 0);
            context.DrawMeshRenderers(m_MeshRenderers.size(), m_MeshRenderers.data(), "MotionVector");
        });
    }

    void RenderPipeline::TAA()
    {
        auto builder = m_RenderGraph->AddPass("TemporalAntialiasing");

        builder.In(m_Resource.MotionVectorTexture);
        builder.InOut(m_Resource.HistoryColorTexture);
        builder.InOut(m_Resource.ColorTarget);

        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.ColorTarget, "_CurrentColorTexture");

            const GfxTextureDesc& desc = m_Resource.ColorTarget.GetDesc();
            context.DispatchComputeByThreadCount(m_TAAShader.get(), "CSMain", desc.Width, desc.Height, 1);

            context.CopyTexture(m_Resource.ColorTarget, 0, 0, m_Resource.HistoryColorTexture, 0, 0);
        });
    }

    void RenderPipeline::Postprocessing()
    {
        auto builder = m_RenderGraph->AddPass("Postprocessing");

        builder.InOut(m_Resource.ColorTarget);
        builder.UseDefaultVariables(false);

        builder.SetRenderFunc([this](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.ColorTarget, "_ColorTexture");

            const GfxTextureDesc& desc = m_Resource.ColorTarget.GetDesc();
            context.DispatchComputeByThreadCount(m_PostprocessingShader.get(), "CSMain", desc.Width, desc.Height, 1);
        });
    }

    void RenderPipeline::HiZ()
    {
        GfxTextureDesc desc{};
        desc.Format = GfxTextureFormat::R32G32_Float;
        desc.Flags = GfxTextureFlags::Mipmaps | GfxTextureFlags::UnorderedAccess;
        desc.Dimension = GfxTextureDimension::Tex2D;
        desc.Width = m_Resource.DepthStencilTarget.GetDesc().Width;
        desc.Height = m_Resource.DepthStencilTarget.GetDesc().Height;
        desc.DepthOrArraySize = 1;
        desc.MSAASamples = 1;
        desc.Filter = GfxTextureFilterMode::Point;
        desc.Wrap = GfxTextureWrapMode::Clamp;
        desc.MipmapBias = 0.0f;

        static int32 hizId = ShaderUtils::GetIdFromString("_HiZTexture");
        m_Resource.HiZTexture = m_RenderGraph->RequestTexture(hizId, desc);

        auto builder = m_RenderGraph->AddPass("HierarchicalZ");

        builder.In(m_Resource.DepthStencilTarget);
        builder.Out(m_Resource.HiZTexture);
        builder.UseDefaultVariables(false);

        builder.SetRenderFunc([this, w = desc.Width, h = desc.Height](RenderGraphContext& context)
        {
            context.SetVariable(m_Resource.DepthStencilTarget, "_InputTexture", GfxTextureElement::Depth);
            context.SetVariable(m_Resource.HiZTexture, "_OutputTexture", GfxTextureElement::Default, 0);
            context.DispatchComputeByThreadCount(m_HiZShader.get(), "CopyDepthMain", w, h, 1);

            for (uint32_t mipLevel = 1; mipLevel < m_Resource.HiZTexture->GetMipLevels(); mipLevel++)
            {
                context.SetVariable(m_Resource.HiZTexture, "_InputTexture", GfxTextureElement::Default, mipLevel - 1);
                context.SetVariable(m_Resource.HiZTexture, "_OutputTexture", GfxTextureElement::Default, mipLevel);

                uint32_t width = std::max(w >> mipLevel, 1u);
                uint32_t height = std::max(h >> mipLevel, 1u);
                context.DispatchComputeByThreadCount(m_HiZShader.get(), "GenMipMain", width, height, 1);
            }
        });
    }
}
