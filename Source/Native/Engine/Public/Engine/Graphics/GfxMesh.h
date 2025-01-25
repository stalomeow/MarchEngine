#pragma once

#include "Engine/Object.h"
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
        RefCountPtr<GfxBufferResource> VertexBuffer;
        RefCountPtr<GfxBufferResource> IndexBuffer;
    };

    template <typename TVertex>
    class GfxBasicMesh : public MarchObject
    {
    public:
        GfxBasicMesh(GfxBufferAllocStrategy allocationStrategy)
            : m_SubMeshes{}
            , m_Vertices{}
            , m_Indices{}
            , m_IsDirty(false)
            , m_AllocationStrategy(allocationStrategy)
            , m_VertexBuffer(GetGfxDevice(), "MeshVertexBuffer")
            , m_IndexBuffer(GetGfxDevice(), "MeshIndexBuffer")
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
                m_IndexBuffer.GetResource(),
            };
        }

        const GfxInputDesc& GetInputDesc() const { return TVertex::GetInputDesc(); }
        uint32_t GetSubMeshCount() const { return static_cast<uint32_t>(m_SubMeshes.size()); }
        const GfxSubMesh& GetSubMesh(uint32_t index) const { return m_SubMeshes[index]; }

    protected:
        std::vector<GfxSubMesh> m_SubMeshes;
        std::vector<TVertex> m_Vertices;
        std::vector<uint16_t> m_Indices;
        bool m_IsDirty;

        GfxBufferAllocStrategy m_AllocationStrategy;
        GfxBuffer m_VertexBuffer;
        GfxBuffer m_IndexBuffer;

        void RecreateBuffersIfDirty()
        {
            if (!m_IsDirty)
            {
                return;
            }

            GfxBufferDesc vbDesc{};
            vbDesc.Stride = sizeof(TVertex);
            vbDesc.Count = static_cast<uint32_t>(m_Vertices.size());
            vbDesc.Usages = GfxBufferUsages::Vertex;

            GfxBufferDesc ibDesc{};
            ibDesc.Stride = sizeof(uint16_t);
            ibDesc.Count = static_cast<uint32_t>(m_Indices.size());
            ibDesc.Usages = GfxBufferUsages::Index;

            m_VertexBuffer.SetData(vbDesc, m_AllocationStrategy, m_Vertices.data());
            m_IndexBuffer.SetData(ibDesc, m_AllocationStrategy, m_Indices.data());
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
        GfxMesh(GfxBufferAllocStrategy allocationStrategy);

        const DirectX::BoundingBox& GetBounds() const { return m_Bounds; }

        void RecalculateNormals();
        void RecalculateTangents();
        void RecalculateBounds();

        static GfxMesh* GetGeometry(GfxMeshGeometry geometry);

    private:
        DirectX::BoundingBox m_Bounds; // Object space bounds
    };
}
