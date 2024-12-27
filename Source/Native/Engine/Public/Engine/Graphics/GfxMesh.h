#pragma once

#include "Engine/Graphics/GfxDevice.h"
#include "Engine/Graphics/GfxBuffer.h"
#include <directx/d3dx12.h>
#include <stdint.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <vector>

namespace march
{
    class GfxInputDesc;

    struct GfxSubMesh
    {
        int32_t BaseVertexLocation;
        uint32_t StartIndexLocation;
        uint32_t IndexCount;
    };

    struct GfxSubMeshDesc
    {
        const GfxInputDesc& InputDesc;
        const GfxSubMesh& SubMesh;

        std::shared_ptr<GfxResource> VertexBufferResource;
        D3D12_VERTEX_BUFFER_VIEW VertexBufferView;

        std::shared_ptr<GfxResource> IndexBufferResource;
        D3D12_INDEX_BUFFER_VIEW IndexBufferView;
    };

    template <typename TVertex>
    class GfxBasicMesh
    {
    public:
        GfxBasicMesh(GfxAllocator allocator)
            : m_Allocator1(allocator)
            , m_UseAllocator2(false)
            , m_SubMeshes{}
            , m_Vertices{}
            , m_Indices{}
            , m_IsDirty(false)
            , m_VertexBuffer{}
            , m_IndexBuffer{}
        {
        }

        GfxBasicMesh(GfxSubAllocator allocator)
            : m_Allocator2(allocator)
            , m_UseAllocator2(true)
            , m_SubMeshes{}
            , m_Vertices{}
            , m_Indices{}
            , m_IsDirty(false)
            , m_VertexBuffer{}
            , m_IndexBuffer{}
        {
        }

        void AddRawSubMesh(const GfxSubMesh& subMesh)
        {
            m_IsDirty = true;
            m_SubMeshes.push_back(subMesh);
        }

        void AddRawVertices(uint32_t numVertices, const TVertex* vertices)
        {
            m_IsDirty = true;
            m_Vertices.insert(m_Vertices.end(), vertices, vertices + numVertices);
        }

        void AddRawIndices(uint32_t numIndices, const uint16_t* indices)
        {
            m_IsDirty = true;
            m_Indices.insert(m_Indices.end(), indices, indices + numIndices);
        }

        void AddSubMesh(uint32_t numVertices, const TVertex* vertices, uint32_t numIndices, const uint16_t* indices)
        {
            GfxSubMesh subMesh{};
            subMesh.BaseVertexLocation = static_cast<int32_t>(m_Vertices.size());
            subMesh.IndexCount = numIndices;
            subMesh.StartIndexLocation = static_cast<uint32_t>(m_Indices.size());

            AddRawSubMesh(subMesh);
            AddRawVertices(numVertices, vertices);
            AddRawIndices(numIndices, indices);
        }

        void ClearSubMeshes()
        {
            if (!m_SubMeshes.empty())
            {
                m_IsDirty = true;
            }

            m_SubMeshes.clear();
            m_Vertices.clear();
            m_Indices.clear();
        }

        GfxSubMeshDesc GetSubMeshDesc(uint32_t index)
        {
            RecreateBuffersIfDirty();

            return GfxSubMeshDesc
            {
                GetInputDesc(),
                GetSubMesh(index),
                m_VertexBuffer.GetResource(),
                m_VertexBuffer.GetVbv(),
                m_IndexBuffer.GetResource(),
                m_IndexBuffer.GetIbv()
            };
        }

        const GfxInputDesc& GetInputDesc() const { return TVertex::GetInputDesc(); }
        uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(m_SubMeshes.size()); }
        const GfxSubMesh& GetSubMesh(uint32_t index) const { return m_SubMeshes[index]; }

    protected:
        union
        {
            GfxAllocator m_Allocator1;
            GfxSubAllocator m_Allocator2;
        };
        bool m_UseAllocator2;

        std::vector<GfxSubMesh> m_SubMeshes;
        std::vector<TVertex> m_Vertices;
        std::vector<uint16_t> m_Indices;
        bool m_IsDirty;

        GfxVertexBuffer<TVertex> m_VertexBuffer;
        GfxIndexBufferUInt16 m_IndexBuffer;

        void RecreateBuffersIfDirty()
        {
            if (!m_IsDirty)
            {
                return;
            }

            GfxDevice* device = GetGfxDevice();

            if (m_UseAllocator2)
            {
                m_VertexBuffer = GfxVertexBuffer<TVertex>(device, static_cast<uint32_t>(m_Vertices.size()), m_Allocator2);
                m_IndexBuffer = GfxIndexBufferUInt16(device, static_cast<uint32_t>(m_Indices.size()), m_Allocator2);
            }
            else
            {
                m_VertexBuffer = GfxVertexBuffer<TVertex>(device, "MeshVertexBuffer", static_cast<uint32_t>(m_Vertices.size()), m_Allocator1);
                m_IndexBuffer = GfxIndexBufferUInt16(device, "MeshIndexBuffer", static_cast<uint32_t>(m_Indices.size()), m_Allocator1);
            }

            m_VertexBuffer.SetData(0, m_Vertices.data(), static_cast<uint32_t>(m_Vertices.size() * sizeof(TVertex)));
            m_IndexBuffer.SetData(0, m_Indices.data(), static_cast<uint32_t>(m_Indices.size() * sizeof(uint16_t)));
            m_IsDirty = false;
        }
    };

    struct GfxMeshVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT4 Tangent;
        DirectX::XMFLOAT2 UV;

        static const GfxInputDesc& GetInputDesc();
    };

    enum class GfxMeshGeometry
    {
        FullScreenTriangle,
        Cube,
        Sphere,
    };

    class GfxMesh final : public GfxBasicMesh<GfxMeshVertex>
    {
        friend class GfxMeshBinding;

    public:
        GfxMesh(GfxAllocator allocator);
        GfxMesh(GfxSubAllocator allocator);

        const DirectX::BoundingBox& GetBounds() const { return m_Bounds; }

        void RecalculateNormals();
        void RecalculateTangents();
        void RecalculateBounds();

        static GfxMesh* GetGeometry(GfxMeshGeometry geometry);

    private:
        DirectX::BoundingBox m_Bounds; // Object space bounds
    };
}
