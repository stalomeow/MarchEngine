#pragma once

#include "Engine/Component.h"
#include "Engine/Rendering/D3D12Impl/GfxMesh.h"
#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <variant>
#include <DirectXCollision.h>
#include <DirectXMath.h>

namespace march
{
    class Material;

    class MeshRenderer : public Component
    {
    public:
        GfxMesh* Mesh;
        std::vector<Material*> Materials;

        MeshRenderer();

        // 获取世界空间 Bounds
        DirectX::BoundingBox GetBounds() const;

        DirectX::XMFLOAT4X4 GetPrevLocalToWorldMatrix() const;
        DirectX::XMMATRIX LoadPrevLocalToWorldMatrix() const;

        void PrepareFrameData();

    private:
        DirectX::XMFLOAT4X4 m_PrevLocalToWorldMatrix;
    };

    class MeshRendererBatch
    {
    public:
        // 相同的可以合批，使用 GPU instancing 绘制
        struct DrawCall
        {
            GfxMesh* Mesh;
            uint32_t SubMeshIndex;

            bool operator==(const DrawCall& other) const
            {
                return Mesh == other.Mesh && SubMeshIndex == other.SubMeshIndex;
            }

            struct Hash
            {
                size_t operator()(const DrawCall& drawCall) const
                {
                    return std::hash<GfxMesh*>{}(drawCall.Mesh) ^ std::hash<uint32_t>{}(drawCall.SubMeshIndex);
                };
            };
        };

        struct InstanceData
        {
            DirectX::XMFLOAT4X4 Matrix;
            DirectX::XMFLOAT4X4 MatrixIT; // 逆转置，用于法线变换
            DirectX::XMFLOAT4X4 MatrixPrev; // 上一帧的矩阵

            static InstanceData Create(const MeshRenderer* renderer);
            static InstanceData Create(const DirectX::XMFLOAT4X4& currMatrix, const DirectX::XMFLOAT4X4& prevMatrix);
        };

        using InstancedDrawCallMap = std::unordered_map<DrawCall, std::vector<InstanceData>, DrawCall::Hash>;
        using FrustumType = std::variant<DirectX::BoundingFrustum, DirectX::BoundingBox, DirectX::BoundingOrientedBox, DirectX::BoundingSphere>;

        void Rebuild(const FrustumType& frustum, size_t numRenderers, MeshRenderer* const* renderers);

        const auto& GetData() const { return m_Data; }

        const auto& GetMeshInputDesc() const { return std::remove_pointer_t<decltype(MeshRenderer::Mesh)>::GetInputDesc(); }

    private:
        std::unordered_map<Material*, InstancedDrawCallMap> m_Data{};
    };
}
