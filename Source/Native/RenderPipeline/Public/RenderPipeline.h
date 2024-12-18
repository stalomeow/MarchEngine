#pragma once

#include "Light.h"
#include "GfxMesh.h"
#include "AssetManger.h"
#include "Material.h"
#include "Shader.h"
#include "GfxUtils.h"
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
        DirectX::XMFLOAT4 CameraPositionWS;
    };

    struct LightConstants
    {
        LightData Lights[LightData::MaxCount];
        int32_t LightCount;
    };

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();

        void ReleaseAssets()
        {
            m_DeferredLitMaterial.reset();
            m_DeferredLitShader.reset();
            m_SkyboxMaterial.reset();
        }

        void Render(Camera* camera, Material* gridGizmoMaterial = nullptr);

        void AddMeshRenderer(MeshRenderer* obj) { m_RenderObjects.push_back(obj); }

        void RemoveMeshRenderer(MeshRenderer* obj)
        {
            auto it = std::find(m_RenderObjects.begin(), m_RenderObjects.end(), obj);

            if (it != m_RenderObjects.end())
            {
                m_RenderObjects.erase(it);
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

        void ImportTextures(int32_t id, GfxRenderTexture* texture);
        void SetCameraGlobalConstantBuffer(const std::string& passName, Camera* camera);
        void SetCameraGlobalConstantBuffer(const std::string& passName, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projectionMatrix);
        void SetLightGlobalConstantBuffer(int32_t id);
        void ResolveMSAA(int32_t id, int32_t resolvedId);
        void ClearTargets(int32_t colorTargetId, int32_t depthStencilTargetId);
        void DeferredLighting(int32_t colorTargetId, int32_t depthStencilTargetId);
        void DrawShadowCasters(int32_t targetId);
        void DrawObjects(int32_t colorTargetId, int32_t depthStencilTargetId, bool wireframe);
        void DrawSceneViewGrid(int32_t colorTargetId, int32_t depthStencilTargetId, Material* material);
        void DrawSkybox(int32_t colorTargetId, int32_t depthStencilTargetId);
        void PrepareTextureForImGui(int32_t id);

    public:
        GfxMesh* m_FullScreenTriangleMesh = nullptr;
        GfxMesh* m_SphereMesh = nullptr;
        std::vector<std::tuple<int32_t, DXGI_FORMAT, bool>> m_GBuffers{};
        asset_ptr<Shader> m_DeferredLitShader = nullptr;
        std::unique_ptr<Material> m_DeferredLitMaterial = nullptr;
        asset_ptr<Material> m_SkyboxMaterial = nullptr;

    private:
        std::vector<MeshRenderer*> m_RenderObjects{};
        std::vector<Light*> m_Lights{};
        std::unique_ptr<RenderGraph> m_RenderGraph = nullptr;
    };
}
