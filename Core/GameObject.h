#pragma once

#include <string>
#include <memory>
#include "Core/Transform.h"
#include "Rendering/Mesh.hpp"
#include "Rendering/Light.h"
#include "Rendering/Resource/GpuBuffer.h"
#include <DirectXMath.h>

namespace dx12demo
{
    struct MaterialData
    {
        DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
        float Roughness = 0.25f;
    };

    class GameObject
    {
    public:
        GameObject()
        {
            m_Transform = std::make_unique<Transform>();
        }
        ~GameObject() = default;

        Transform* GetTransform() const { return m_Transform.get(); }
        SimpleMesh* GetMesh() const { return m_Mesh.get(); }
        Light* GetLight() const { return m_Light.get(); }
        ConstantBuffer<MaterialData>* GetMaterialBuffer() const { return m_MaterialBuffer.get(); }
        MaterialData& GetMaterialData() { return m_MaterialData; }

        void AddMesh()
        {
            m_Mesh = std::make_unique<SimpleMesh>();
            m_MaterialBuffer = std::make_unique<ConstantBuffer<MaterialData>>(L"Material cbuffer", 1);
            m_MaterialBuffer->SetData(0, m_MaterialData);
        }

        void AddLight()
        {
            m_Light = std::make_unique<Light>();
        }

    public:
        bool IsActive = true;
        std::string Name = u8"New GameObject";

    private:
        std::unique_ptr<Transform> m_Transform;
        std::unique_ptr<SimpleMesh> m_Mesh = nullptr;
        std::unique_ptr<Light> m_Light = nullptr;
        std::unique_ptr<ConstantBuffer<MaterialData>> m_MaterialBuffer = nullptr;

        MaterialData m_MaterialData{};
    };
}
