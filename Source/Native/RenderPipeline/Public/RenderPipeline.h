#pragma once

#include <directx/d3dx12.h>
#include "Light.h"
#include "GfxMesh.h"
#include <dxgi.h>
#include <dxgi1_4.h>
#include <vector>
#include <memory>

namespace march
{
    class Camera;
    class RenderObject;
    class Material;
    class RenderGraph;

    struct PerObjConstants
    {
        DirectX::XMFLOAT4X4 WorldMatrix;
    };

    struct PerPassConstants
    {
        DirectX::XMFLOAT4X4 ViewMatrix;
        DirectX::XMFLOAT4X4 ProjectionMatrix;
        DirectX::XMFLOAT4X4 ViewProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewMatrix;
        DirectX::XMFLOAT4X4 InvProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewProjectionMatrix;
        DirectX::XMFLOAT4 Time; // elapsed time, delta time, unused, unused
        DirectX::XMFLOAT4 CameraPositionWS;

        LightData Lights[LightData::MaxCount];
        int LightCount;
    };

    class RenderPipeline
    {
    public:
        RenderPipeline();
        ~RenderPipeline() = default;

        void Render(Camera* camera, Material* gridGizmoMaterial = nullptr);

        void AddRenderObject(RenderObject* obj) { m_RenderObjects.push_back(obj); }

        void RemoveRenderObject(RenderObject* obj)
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

    private:
        std::vector<RenderObject*> m_RenderObjects{};
        std::vector<Light*> m_Lights{};
        std::unique_ptr<GfxMesh> m_FullScreenTriangleMesh = nullptr;
        std::unique_ptr<RenderGraph> m_RenderGraph = nullptr;
    };
}
