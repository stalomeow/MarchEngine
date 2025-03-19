#pragma once

#include "Engine/Rendering/Light.h"
#include "Engine/Rendering/RenderGraph.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/AssetManger.h"
#include "Engine/InlineArray.h"
#include "Engine/Ints.h"
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
#include <string>

namespace march
{
    enum class GBufferElements
    {
        None = 0,

        Albedo = 1 << 0,
        Metallic = 1 << 1,
        Roughness = 1 << 2,
        Normal = 1 << 3,
        Emission = 1 << 4,
        Occlusion = 1 << 5,

        BRDF = Albedo | Metallic | Roughness,

        GBuffer0 = Albedo | Metallic,
        GBuffer1 = Normal | Roughness,
        GBuffer2 = Occlusion,
        GBuffer3 = Emission,

        All = GBuffer0 | GBuffer1 | GBuffer2 | GBuffer3,
    };

    DEFINE_ENUM_FLAG_OPERATORS(GBufferElements);

    struct RenderPipelineResource
    {
        static constexpr size_t NumGBuffers = 4;

        static constexpr GBufferElements GBufferData[NumGBuffers] =
        {
            GBufferElements::GBuffer0,
            GBufferElements::GBuffer1,
            GBufferElements::GBuffer2,
            GBufferElements::GBuffer3,
        };

        static constexpr GfxTextureFormat GBufferFormats[NumGBuffers] =
        {
            GfxTextureFormat::R8G8B8A8_UNorm,
            GfxTextureFormat::R8G8B8A8_UNorm,
            GfxTextureFormat::R8_UNorm,
            GfxTextureFormat::R11G11B10_Float, // HDR
        };

        static constexpr GfxTextureFlags GBufferFlags[NumGBuffers] =
        {
            GfxTextureFlags::SRGB,
            GfxTextureFlags::None,
            GfxTextureFlags::None,
            GfxTextureFlags::None,
        };

        TextureHandle ColorTarget;
        TextureHandle DepthStencilTarget;
        TextureHandle HistoryColorTexture;
        TextureHandle GBuffers[NumGBuffers];
        TextureHandle MotionVectorTexture;

        BufferHandle CbCamera;
        BufferHandle CbLight;
        BufferHandle DirectionalLights;
        BufferHandle PunctualLights;
        BufferHandle ClusterPunctualLightRanges;
        BufferHandle ClusterPunctualLightIndices;
        BufferHandle VisibleLightCounter;
        BufferHandle MaxClusterZIds;

        BufferHandle EnvDiffuseSH9Coefs;
        TextureHandle EnvSpecularRadianceMap;
        TextureHandle EnvSpecularBRDFLUT;

        BufferHandle CbSSAO;
        TextureHandle SSAOMap;
        TextureHandle SSAOMapTemp;
        TextureHandle SSAORandomVectorMap;

        BufferHandle CbShadow;
        TextureHandle ShadowMap;

        TextureHandle HiZTexture;
        TextureHandle SSGITexture;
        TextureHandle SSGITextureTemp;

        void Reset();
        void RequestGBuffers(RenderGraph* graph, uint32_t width, uint32_t height);
        InlineArray<TextureHandle, NumGBuffers> GetGBuffers(GBufferElements elements) const;
    };

    class Camera;

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();

        void InitResourcesIfNot();

        void RenderSingleCamera(Camera* camera);
        void Render();

        void AddMeshRenderer(MeshRenderer* obj) { m_Renderers.push_back(obj); }

        void RemoveMeshRenderer(MeshRenderer* obj)
        {
            auto it = std::find(m_Renderers.begin(), m_Renderers.end(), obj);

            if (it != m_Renderers.end())
            {
                m_Renderers.erase(it);
            }
        }

        void AddLight(Light* light) { m_Lights.push_back(light); }

        void RemoveLight(Light* light)
        {
            auto it = std::find(m_Lights.begin(), m_Lights.end(), light);

            if (it != m_Lights.end())
            {
                m_Lights.erase(it);
            }
        }

        void PrepareFrameData();

        void SetSkyboxMaterial(Material* material) { m_SkyboxMaterial = material; }

        BufferHandle CreateCameraConstantBuffer(const std::string& name, Camera* camera);
        BufferHandle CreateCameraConstantBuffer(const std::string& name, const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projectionMatrix);

        void CullLights();
        void DeferredLighting(const DirectX::XMFLOAT4X4& shadowMatrix, float depth2RadialScale);
        void DrawShadowCasters(DirectX::XMFLOAT4X4& shadowMatrix, float& depth2RadialScale);
        void ClearAndDrawObjects(bool wireframe);
        void DrawSceneViewGrid();
        void DrawSkybox();

        void GenerateSSAORandomVectorMap();
        void SSAO();

        void CreateLightResources();
        void CreateEnvLightResources();
        void BakeEnvLight(GfxTexture* radianceMap, float diffuseIntensityMultiplier, float specularIntensityMultiplier);
        void GenerateEnvSpecularBRDFLUT();

        void DrawMotionVector();
        void TAA();

        void Postprocessing();

        void Hiz();
        void SSGI();

    private:

        asset_ptr<Shader> m_DeferredLitShader = nullptr;
        std::unique_ptr<Material> m_DeferredLitMaterial = nullptr;
        asset_ptr<Shader> m_SceneViewGridShader = nullptr;
        std::unique_ptr<Material> m_SceneViewGridMaterial = nullptr;
        asset_ptr<Shader> m_CameraMotionVectorShader = nullptr;
        std::unique_ptr<Material> m_CameraMotionVectorMaterial = nullptr;
        asset_ptr<ComputeShader> m_SSAOShader = nullptr;
        asset_ptr<ComputeShader> m_CullLightShader = nullptr;
        asset_ptr<ComputeShader> m_DiffuseIrradianceShader = nullptr;
        asset_ptr<ComputeShader> m_SpecularIBLShader = nullptr;
        asset_ptr<ComputeShader> m_TAAShader = nullptr;
        asset_ptr<ComputeShader> m_PostprocessingShader = nullptr;
        asset_ptr<ComputeShader> m_HizShader = nullptr;
        asset_ptr<ComputeShader> m_SSGIShader = nullptr;
        std::unique_ptr<GfxExternalTexture> m_SSAORandomVectorMap = nullptr;

        std::unique_ptr<GfxBuffer> m_ClusterPunctualLightRangesBuffer = nullptr;
        std::unique_ptr<GfxBuffer> m_ClusterPunctualLightIndicesBuffer = nullptr;
        std::unique_ptr<GfxBuffer> m_VisibleLightCounterBuffer = nullptr;
        std::unique_ptr<GfxBuffer> m_MaxClusterZIdsBuffer = nullptr;

        std::unique_ptr<GfxBuffer> m_EnvDiffuseSH9Coefs = nullptr;
        std::unique_ptr<GfxTexture> m_EnvSpecularRadianceMap = nullptr;
        asset_ptr<GfxExternalTexture> m_EnvSpecularBRDFLUT = nullptr;

        Material* m_SkyboxMaterial = nullptr;

        RenderPipelineResource m_Resource{};

        std::vector<MeshRenderer*> m_Renderers{};
        std::vector<Light*> m_Lights{};
        std::unique_ptr<RenderGraph> m_RenderGraph = nullptr;
        bool m_IsResourceLoaded = false;
    };
}
