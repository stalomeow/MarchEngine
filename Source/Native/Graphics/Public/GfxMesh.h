#pragma once

#include "GfxBuffer.h"
#include <directx/d3dx12.h>
#include <stdint.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>

namespace march
{
    class GfxInputDesc;

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
        GfxMesh(GfxAllocator allocator);
        GfxMesh(GfxSubAllocator allocator);

        uint32_t GetSubMeshCount() const;
        const GfxSubMesh& GetSubMesh(uint32_t index) const;
        void ClearSubMeshes();

        const DirectX::BoundingBox& GetBounds() const { return m_Bounds; }

        const GfxInputDesc& GetInputDesc();
        void GetBufferViews(D3D12_VERTEX_BUFFER_VIEW* pOutVbv, D3D12_INDEX_BUFFER_VIEW* pOutIbv);
        void RecalculateNormals();
        void RecalculateTangents();
        void RecalculateBounds();

        void AddSubMesh(const std::vector<GfxMeshVertex>& vertices, const std::vector<uint16_t>& indices);

        static GfxMesh* GetGeometry(GfxMeshGeometry geometry);

        GfxMesh(const GfxMesh&) = delete;
        GfxMesh& operator=(const GfxMesh&) = delete;

    private:
        union
        {
            GfxAllocator m_Allocator1;
            GfxSubAllocator m_Allocator2;
        };
        bool m_UseAllocator2;

        std::vector<GfxSubMesh> m_SubMeshes;
        std::vector<GfxMeshVertex> m_Vertices;
        std::vector<uint16_t> m_Indices;
        DirectX::BoundingBox m_Bounds; // Object space bounds
        bool m_IsDirty;

        GfxVertexBuffer<GfxMeshVertex> m_VertexBuffer;
        GfxIndexBufferUInt16 m_IndexBuffer;
    };
}
