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

        BufferHandle CreateCameraConstantBuffer(const std::string& passName, Camera* camera, DirectX::XMFLOAT4X4& viewProjMatrix);
        BufferHandle CreateCameraConstantBuffer(const std::string& passName, const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT4X4& viewMatrix, const DirectX::XMFLOAT4X4& projectionMatrix, DirectX::XMFLOAT4X4& viewProjMatrix);
        BufferHandle CreateLightConstantBuffer();
        void ClearTargets(const TextureHandle& colorTarget, const TextureHandle& depthStencilTarget);
        void DeferredLighting(int32_t colorTargetId, int32_t depthStencilTargetId, int32 screenSpaceShadowMapId);
        TextureHandle DrawShadowCasters(DirectX::XMFLOAT4X4& shadowMatrix);
        void DrawObjects(const BufferHandle& cbCamera, const TextureHandle& colorTarget, const TextureHandle& depthStencilTarget, bool wireframe);
        void DrawSceneViewGrid(int32_t colorTargetId, int32_t depthStencilTargetId, Material* material);
        void DrawSkybox(int32_t colorTargetId, int32_t depthStencilTargetId);
        void ResolveMSAA(int32_t sourceId, int32_t destinationId);

        void ScreenSpaceShadow(const DirectX::XMFLOAT4X4& shadowMatrix, int32_t cameraColorTargetId, int32_t shadowMapId, int32_t destinationId);

        void TestCompute();

    public:
        GfxMesh* m_FullScreenTriangleMesh = nullptr;
        GfxMesh* m_SphereMesh = nullptr;
        std::vector<std::tuple<int32_t, DXGI_FORMAT, bool>> m_GBuffers{};
        asset_ptr<Shader> m_DeferredLitShader = nullptr;
        std::unique_ptr<Material> m_DeferredLitMaterial = nullptr;
        asset_ptr<Material> m_SkyboxMaterial = nullptr;
        asset_ptr<Shader> m_ScreenSpaceShadowShader = nullptr;
        asset_ptr<ComputeShader> m_ComputeShader = nullptr;
        std::unique_ptr<Material> m_ScreenSpaceShadowMaterial = nullptr;

    private:
        std::vector<MeshRenderer*> m_Renderers{};
        std::vector<Light*> m_Lights{};
        std::unique_ptr<RenderGraph> m_RenderGraph = nullptr;
    };
}
