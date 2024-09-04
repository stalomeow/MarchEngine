#pragma once

#include <directx/d3dx12.h>
#include "Rendering/Mesh.hpp"
#include "Rendering/DescriptorHeap.h"
#include "Rendering/Command/CommandBuffer.h"
#include "Rendering/Light.h"
#include "Rendering/RenderObject.h"
#include "Rendering/Resource/Texture.h"
#include "Scripting/ScriptTypes.h"
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <memory>
#include <tuple>
#include <functional>
#include <vector>

namespace dx12demo
{
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
        RenderPipeline(int width, int height);
        ~RenderPipeline() = default;

        void Resize(int width, int height);
        void Render(CommandBuffer* cmd);

        bool GetEnableMSAA() const { return m_EnableMSAA; }
        void SetEnableMSAA(bool value);

        bool GetIsWireframe() const { return m_IsWireframe; }
        void SetIsWireframe(bool value) { m_IsWireframe = value; }

        ID3D12Resource* GetResolvedColorTarget() const { return m_ResolvedColorTarget.Get(); }

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
        void CheckMSAAQuailty();
        void CreateDescriptorHeaps();
        void CreateColorAndDepthStencilTarget(int width, int height);

        D3D12_CPU_DESCRIPTOR_HANDLE GetColorRenderTargetView();
        D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilTargetView();

    private:
        bool m_EnableMSAA = true;
        UINT m_MSAAQuality;

        std::unique_ptr<DescriptorHeap> m_RtvHeap;
        std::unique_ptr<DescriptorHeap> m_DsvHeap;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_ColorTarget;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_ResolvedColorTarget;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_DepthStencilTarget;
        D3D12_RESOURCE_STATES m_LastColorTargetState;
        D3D12_RESOURCE_STATES m_LastResolvedColorTargetState;

        std::unique_ptr<DescriptorHeap> m_SrvHeap;
        std::unique_ptr<DescriptorHeap> m_SamplerHeap;

        int m_RenderTargetWidth;
        int m_RenderTargetHeight;
        D3D12_VIEWPORT m_Viewport;
        D3D12_RECT m_ScissorRect;

        bool m_IsWireframe = false;

        float m_Theta = 1.5f * DirectX::XM_PI;
        float m_Phi = DirectX::XM_PIDIV4;
        float m_Radius = 5.0f;

        std::vector<RenderObject*> m_RenderObjects{};
        std::vector<Light*> m_Lights{};

    private:
        DXGI_FORMAT m_DepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        UINT m_MSAASampleCount = 4;
    };

    namespace binding
    {
        inline CSHARP_API(void) RenderPipeline_AddRenderObject(RenderPipeline* pPipeline, RenderObject* pObject)
        {
            pPipeline->AddRenderObject(pObject);
        }

        inline CSHARP_API(void) RenderPipeline_RemoveRenderObject(RenderPipeline* pPipeline, RenderObject* pObject)
        {
            pPipeline->RemoveRenderObject(pObject);
        }

        inline CSHARP_API(void) RenderPipeline_AddLight(RenderPipeline* pPipeline, Light* pLight)
        {
            pPipeline->AddLight(pLight);
        }

        inline CSHARP_API(void) RenderPipeline_RemoveLight(RenderPipeline* pPipeline, Light* pLight)
        {
            pPipeline->RemoveLight(pLight);
        }
    }
};
