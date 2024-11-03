#pragma once

#include "GfxBuffer.h"
#include <directx/d3dx12.h>
#include <stdint.h>
#include <DirectXMath.h>
#include <vector>
#include <memory>

namespace march
{
    struct GfxMeshVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT4 Tangent;
        DirectX::XMFLOAT2 UV;

        GfxMeshVertex() = default;
        GfxMeshVertex(float px, float py, float pz, float nx, float ny, float nz, float tx, float ty, float tz, float tw, float u, float v)
            : Position(px, py, pz), Normal(nx, ny, nz), Tangent(tx, ty, tz, tw), UV(u, v) { }
    };

    struct GfxSubMesh
    {
        int32_t BaseVertexLocation;
        uint32_t StartIndexLocation;
        uint32_t IndexCount;
    };

    enum class GfxMeshGeometry
    {
        FullScreenTriangle,
        Cube,
        Sphere,
    };

    class GfxMeshBinding;

    class GfxMesh final
    {
        friend GfxMeshBinding;

    public:
        GfxMesh();

        uint32_t GetSubMeshCount() const;
        const GfxSubMesh& GetSubMesh(uint32_t index) const;
        void ClearSubMeshes();

        void GetBufferViews(D3D12_VERTEX_BUFFER_VIEW& vbv, D3D12_INDEX_BUFFER_VIEW& ibv);
        void RecalculateNormals();
        void RecalculateTangents();

        void AddSubMesh(const std::vector<GfxMeshVertex>& vertices, const std::vector<uint16_t>& indices);

        static int32_t GetPipelineInputDescId();
        static D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology();

        static GfxMesh* GetGeometry(GfxMeshGeometry geometry);

        GfxMesh(const GfxMesh&) = delete;
        GfxMesh& operator=(const GfxMesh&) = delete;

    private:
        std::vector<GfxSubMesh> m_SubMeshes;
        std::vector<GfxMeshVertex> m_Vertices;
        std::vector<uint16_t> m_Indices;
        bool m_IsDirty;

        std::unique_ptr<GfxVertexBuffer<GfxMeshVertex>> m_VertexBuffer;
        std::unique_ptr<GfxIndexBuffer<uint16_t>> m_IndexBuffer;
    };
}
