#pragma once

#include "Engine/Rendering/Light.h"
#include "Engine/Rendering/RenderGraph.h"
#include "Engine/Rendering/D3D12.h"
#include "Engine/AssetManger.h"
#include "Engine/Ints.h"
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
#include <imgui.h>
#include <string>

namespace march
{
    class Camera;
    class MeshRenderer;
    class RenderGraph;
    class GfxRenderTexture;

    struct CameraConstants
    {
        DirectX::XMFLOAT4X4 ViewMatrix;
        DirectX::XMFLOAT4X4 ProjectionMatrix;
        DirectX::XMFLOAT4X4 ViewProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewMatrix;
        DirectX::XMFLOAT4X4 InvProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewProjectionMatrix;
    };

    struct LightConstants
    {
        DirectX::XMINT4 NumLights;   // x: numDirectionalLights, y: numPunctualLights
        DirectX::XMINT4 NumClusters; // xyz: numClusters
    };

    struct ShadowConstants
    {
        DirectX::XMFLOAT4X4 ShadowMatrix;
    };

    struct RenderPipelineResource
    {
        TextureHandle ColorTarget;
        TextureHandle DepthStencilTarget;

        std::vector<TextureHandle> GBuffers;

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

        void Reset()
        {
            ColorTarget = {};
            DepthStencilTarget = {};
            GBuffers.clear();
            CbCamera = {};
            CbLight = {};
            DirectionalLights = {};
            PunctualLights = {};
            ClusterPunctualLightRanges = {};
            ClusterPunctualLightIndices = {};
            VisibleLightCounter = {};
            MaxClusterZIds = {};
            EnvDiffuseSH9Coefs = {};
            EnvSpecularRadianceMap = {};
            EnvSpecularBRDFLUT = {};
            CbSSAO = {};
            SSAOMap = {};
            SSAOMapTemp = {};
            SSAORandomVectorMap = {};
            CbShadow = {};
            ShadowMap = {};
        }
    };

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();

        void InitResourcesIfNot();

        void Render(Camera* camera, Material* gridGizmoMaterial = nullptr);

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

        void SetSkyboxMaterial(Material* material) { m_SkyboxMaterial = material; }

        BufferHandle CreateCameraConstantBuffer(const std::string& name, Camera* camera);
        BufferHandle CreateCameraConstantBuffer(const std::string& name, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projectionMatrix);

        void CullLights();
        void DeferredLighting(const DirectX::XMFLOAT4X4& shadowMatrix);
        void DrawShadowCasters(DirectX::XMFLOAT4X4& shadowMatrix);
        void ClearAndDrawObjects(bool wireframe);
        void DrawSceneViewGrid(Material* material);
        void DrawSkybox();

        void GenerateSSAORandomVectorMap();
        void SSAO();

        void CreateLightResources();
        void CreateEnvLightResources();
        void BakeEnvLight(GfxTexture* radianceMap, float diffuseIntensityMultiplier, float specularIntensityMultiplier);

        asset_ptr<Shader> m_DeferredLitShader = nullptr;
        std::unique_ptr<Material> m_DeferredLitMaterial = nullptr;
        asset_ptr<ComputeShader> m_SSAOShader = nullptr;
        asset_ptr<ComputeShader> m_CullLightShader = nullptr;
        asset_ptr<ComputeShader> m_DiffuseIrradianceShader = nullptr;
        asset_ptr<ComputeShader> m_SpecularIBLShader = nullptr;
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

    private:
        std::vector<MeshRenderer*> m_Renderers{};
        std::vector<Light*> m_Lights{};
        std::unique_ptr<RenderGraph> m_RenderGraph = nullptr;
        bool m_IsResourceLoaded = false;
    };
}
