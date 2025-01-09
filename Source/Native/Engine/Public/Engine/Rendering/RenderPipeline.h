#pragma once

#include "Engine/Rendering/Light.h"
#include "Engine/Graphics/GfxMesh.h"
#include "Engine/Graphics/GfxBuffer.h"
#include "Engine/Graphics/Material.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Graphics/GfxUtils.h"
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
        DirectX::XMFLOAT4 CameraPositionWS;
    };

    struct LightConstants
    {
        LightData Lights[LightData::MaxCount];
        int32_t LightCount;
    };

    struct ShadowConstants
    {
        DirectX::XMFLOAT4X4 ShadowMatrix;
    };

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline();

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

        void ImportTextures(int32_t id, GfxRenderTexture* texture);
        DirectX::XMFLOAT4X4 SetCameraGlobalConstantBuffer(const std::string& passName, GfxConstantBuffer<CameraConstants>* buffer, Camera* camera);
        DirectX::XMFLOAT4X4 SetCameraGlobalConstantBuffer(const std::string& passName, GfxConstantBuffer<CameraConstants>* buffer, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projectionMatrix);
        void SetLightGlobalConstantBuffer(int32_t id);
        void ClearTargets(int32_t colorTargetId, int32_t depthStencilTargetId);
        void DeferredLighting(int32_t colorTargetId, int32_t depthStencilTargetId);
        DirectX::XMFLOAT4X4 DrawShadowCasters(int32_t targetId);
        void DrawObjects(int32_t colorTargetId, int32_t depthStencilTargetId, bool wireframe);
        void DrawSceneViewGrid(int32_t colorTargetId, int32_t depthStencilTargetId, Material* material);
        void DrawSkybox(int32_t colorTargetId, int32_t depthStencilTargetId);
        void ResolveMSAA(int32_t sourceId, int32_t destinationId);

        void ScreenSpaceShadow(const DirectX::XMFLOAT4X4& shadowMatrix, int32_t cameraColorTargetId, int32_t shadowMapId, int32_t destinationId);

    public:
        GfxMesh* m_FullScreenTriangleMesh = nullptr;
        GfxMesh* m_SphereMesh = nullptr;
        std::vector<std::tuple<int32_t, DXGI_FORMAT, bool>> m_GBuffers{};
        asset_ptr<Shader> m_DeferredLitShader = nullptr;
        std::unique_ptr<Material> m_DeferredLitMaterial = nullptr;
        asset_ptr<Material> m_SkyboxMaterial = nullptr;
        asset_ptr<Shader> m_ScreenSpaceShadowShader = nullptr;
        std::unique_ptr<Material> m_ScreenSpaceShadowMaterial = nullptr;

        GfxConstantBuffer<CameraConstants> m_CameraConstantBuffer{};
        GfxConstantBuffer<CameraConstants> m_ShadowCameraConstantBuffer{};
        GfxConstantBuffer<LightConstants> m_LightConstantBuffer{};
        GfxConstantBuffer<ShadowConstants> m_ShadowConstantBuffer{};

    private:
        std::vector<MeshRenderer*> m_Renderers{};
        std::vector<Light*> m_Lights{};
        std::unique_ptr<RenderGraph> m_RenderGraph = nullptr;
    };
}
