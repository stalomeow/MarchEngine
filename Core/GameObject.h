#pragma once

#include <string>
#include <memory>
#include "Core/Transform.h"
#include "Rendering/Mesh.hpp"

namespace dx12demo
{
    class GameObject
    {
    public:
        GameObject()
        {
            m_Transform = std::make_unique<Transform>();
            m_Mesh = std::make_unique<SimpleMesh>();
            m_Mesh->AddSubMeshCube();
        }
        ~GameObject() = default;

        Transform* GetTransform() const { return m_Transform.get(); }
        Mesh* GetMesh() const { return m_Mesh.get(); }

    public:
        bool IsActive = true;
        std::string Name = u8"New GameObject";

    private:
        std::unique_ptr<Transform> m_Transform;
        std::unique_ptr<SimpleMesh> m_Mesh;
    };
}
