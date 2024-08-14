#pragma once

#include <directx/d3dx12.h>
#include "Rendering/DxException.h"
#include "Rendering/Command/CommandBuffer.h"
#include "Rendering/Resource/GpuBuffer.h"
#include "Rendering/GfxManager.h"
#include "Scripting/ScriptTypes.h"
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <vector>
#include <memory>

namespace dx12demo
{
    class Mesh
    {
    protected:
        struct SubMesh
        {
            INT BaseVertexLocation;
            UINT StartIndexLocation;
            UINT IndexCount;
        };

    protected:
        Mesh() = default;
        Mesh(const Mesh& rhs) = delete;
        Mesh& operator=(const Mesh& rhs) = delete;

    public:
        virtual ~Mesh() = default;

        // subMeshIndex = -1 means draw all sub-meshes
        virtual void Draw(CommandBuffer* cmd, int subMeshIndex = -1) = 0;
    };

    template<typename TVertex, typename TIndex>
    class MeshImpl : public Mesh
    {
        static_assert(sizeof(TIndex) == 2 || sizeof(TIndex) == 4, "TIndex must be 2 or 4 bytes in size.");

    public:
        MeshImpl(D3D12_PRIMITIVE_TOPOLOGY topology);
        MeshImpl(const MeshImpl& rhs) = delete;
        MeshImpl& operator=(const MeshImpl& rhs) = delete;
        ~MeshImpl() override = default;

        void ClearSubMeshes();
        void AddSubMesh(const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices);
        void RecalculateNormals();

        void Draw(CommandBuffer* cmd, int subMeshIndex = -1) override;

        static D3D12_INPUT_LAYOUT_DESC VertexInputLayout()
        {
            D3D12_INPUT_LAYOUT_DESC desc = {};
            desc.pInputElementDescs = TVertex::InputDesc;
            desc.NumElements = _countof(TVertex::InputDesc);
            return desc;
        }

        int SubMeshCount() const { return m_SubMeshes.size(); }

        D3D12_PRIMITIVE_TOPOLOGY Topology() const { return m_Topology; }

    private:
        D3D12_PRIMITIVE_TOPOLOGY m_Topology;

        std::vector<SubMesh> m_SubMeshes{};
        std::vector<TVertex> m_Vertices{};
        std::vector<TIndex> m_Indices{};
        bool m_IsDirty = false;

        std::unique_ptr<VertexBuffer<TVertex>> m_VertexBuffer = nullptr;
        std::unique_ptr<IndexBuffer<TIndex>> m_IndexBuffer = nullptr;
    };

    template<typename TVertex, typename TIndex>
    MeshImpl<TVertex, TIndex>::MeshImpl(D3D12_PRIMITIVE_TOPOLOGY topology)
        : m_Topology(topology)
    {
    }

    namespace
    {
        using namespace Microsoft::WRL;

        void UploadToGpuBuffer(CommandBuffer* cmd, GpuBuffer* dest, const void* pData, UINT64 size)
        {
            D3D12_SUBRESOURCE_DATA subResData = {};
            subResData.pData = pData;
            subResData.RowPitch = size;
            subResData.SlicePitch = size;

            // Vertex Buffer 和 Index Buffer 都放在默认堆，优化性能
            // 上传数据时，借助上传堆中转
            UploadHeapSpan<BYTE> span = cmd->AllocateTempUploadHeap(size);

            dest->ResourceBarrier(cmd->GetList(), D3D12_RESOURCE_STATE_COPY_DEST);
            UpdateSubresources<1>(cmd->GetList(), dest->GetResource(),
                span.GetResource(), span.GetOffsetInResource(), 0, 1, &subResData);
            dest->ResourceBarrier(cmd->GetList(), D3D12_RESOURCE_STATE_GENERIC_READ);
        }
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::ClearSubMeshes()
    {
        if (m_SubMeshes.size() > 0)
        {
            m_IsDirty = true;
        }

        m_SubMeshes.clear();
        m_Vertices.clear();
        m_Indices.clear();
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::AddSubMesh(const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices)
    {
        SubMesh subMesh;
        subMesh.BaseVertexLocation = m_Vertices.size();
        subMesh.IndexCount = indices.size();
        subMesh.StartIndexLocation = m_Indices.size();

        m_IsDirty = true;
        m_SubMeshes.push_back(subMesh);
        m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
        m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::RecalculateNormals()
    {
        m_IsDirty = true;
        assert(m_Topology == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        for (TVertex& v : m_Vertices)
        {
            v.Normal = { 0.0, 0.0, 0.0 };
        }

        for (int i = 0; i < m_Indices.size() / 3; i++)
        {
            TVertex& v0 = m_Vertices[m_Indices[i * 3 + 0]];
            TVertex& v1 = m_Vertices[m_Indices[i * 3 + 1]];
            TVertex& v2 = m_Vertices[m_Indices[i * 3 + 2]];

            DirectX::XMVECTOR p0 = DirectX::XMLoadFloat3(&v0.Position);
            DirectX::XMVECTOR p1 = DirectX::XMLoadFloat3(&v1.Position);
            DirectX::XMVECTOR p2 = DirectX::XMLoadFloat3(&v2.Position);
            DirectX::XMVECTOR vec1 = DirectX::XMVectorSubtract(p1, p0);
            DirectX::XMVECTOR vec2 = DirectX::XMVectorSubtract(p2, p0);
            DirectX::XMVECTOR normal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(vec1, vec2));

            DirectX::XMStoreFloat3(&v0.Normal, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&v0.Normal), normal));
            DirectX::XMStoreFloat3(&v1.Normal, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&v1.Normal), normal));
            DirectX::XMStoreFloat3(&v2.Normal, DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&v2.Normal), normal));
        }

        for (TVertex& v : m_Vertices)
        {
            DirectX::XMStoreFloat3(&v.Normal, DirectX::XMVector3Normalize(DirectX::XMLoadFloat3(&v.Normal)));
        }
    }

    template<typename TVertex, typename TIndex>
    void MeshImpl<TVertex, TIndex>::Draw(CommandBuffer* cmd, int subMeshIndex)
    {
        if (m_IsDirty)
        {
            m_IsDirty = false;

            m_VertexBuffer = std::make_unique<VertexBuffer<TVertex>>(L"Mesh Vertex Buffer", m_Vertices.size());
            m_IndexBuffer = std::make_unique<IndexBuffer<TIndex>>(L"Mesh Index Buffer", m_Indices.size());

            UploadToGpuBuffer(cmd, m_VertexBuffer.get(), m_Vertices.data(), m_VertexBuffer->GetSize());
            UploadToGpuBuffer(cmd, m_IndexBuffer.get(), m_Indices.data(), m_IndexBuffer->GetSize());
        }

        cmd->GetList()->IASetVertexBuffers(0, 1, &m_VertexBuffer->GetView());
        cmd->GetList()->IASetIndexBuffer(&m_IndexBuffer->GetView());
        cmd->GetList()->IASetPrimitiveTopology(m_Topology);

        if (subMeshIndex == -1)
        {
            for (auto& subMesh : m_SubMeshes)
            {
                cmd->GetList()->DrawIndexedInstanced(subMesh.IndexCount, 1,
                    subMesh.StartIndexLocation, subMesh.BaseVertexLocation, 0);
            }
        }
        else
        {
            auto& subMesh = m_SubMeshes[subMeshIndex];
            cmd->GetList()->DrawIndexedInstanced(subMesh.IndexCount, 1,
                subMesh.StartIndexLocation, subMesh.BaseVertexLocation, 0);
        }
    }

    struct SimpleMeshVertex
    {
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 Tangent;
        DirectX::XMFLOAT2 UV;

        static constexpr D3D12_INPUT_ELEMENT_DESC InputDesc[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        SimpleMeshVertex() = default;

        SimpleMeshVertex(float x, float y, float z, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v)
            : Position(x, y, z), Normal(nx, ny, nz), Tangent(tx, ty, tz), UV(u, v) {}
    };

    class SimpleMesh : public MeshImpl<SimpleMeshVertex, std::uint16_t>
    {
    public:
        SimpleMesh() : MeshImpl<SimpleMeshVertex, std::uint16_t>(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST) {}
        SimpleMesh(const SimpleMesh& rhs) = delete;
        SimpleMesh& operator=(const SimpleMesh& rhs) = delete;
        ~SimpleMesh() override = default;

        void AddSubMeshCube(float width = 1, float height = 1, float depth = 1)
        {
            std::vector<SimpleMeshVertex> vertices;
            std::vector<std::uint16_t> i(36);

            float w2 = 0.5f * width;
            float h2 = 0.5f * height;
            float d2 = 0.5f * depth;

            // Fill in the front face vertex data.
            vertices.emplace_back(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            vertices.emplace_back(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vertices.emplace_back(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
            vertices.emplace_back(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

            // Fill in the back face vertex data.
            vertices.emplace_back(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
            vertices.emplace_back(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            vertices.emplace_back(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vertices.emplace_back(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

            // Fill in the top face vertex data.
            vertices.emplace_back(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            vertices.emplace_back(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vertices.emplace_back(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
            vertices.emplace_back(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

            // Fill in the bottom face vertex data.
            vertices.emplace_back(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
            vertices.emplace_back(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            vertices.emplace_back(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            vertices.emplace_back(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

            // Fill in the left face vertex data.
            vertices.emplace_back(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
            vertices.emplace_back(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
            vertices.emplace_back(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
            vertices.emplace_back(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

            // Fill in the right face vertex data.
            vertices.emplace_back(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
            vertices.emplace_back(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
            vertices.emplace_back(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
            vertices.emplace_back(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

            // Fill in the front face index data
            i[0] = 0; i[1] = 1; i[2] = 2;
            i[3] = 0; i[4] = 2; i[5] = 3;

            // Fill in the back face index data
            i[6] = 4; i[7] = 5; i[8] = 6;
            i[9] = 4; i[10] = 6; i[11] = 7;

            // Fill in the top face index data
            i[12] = 8; i[13] = 9; i[14] = 10;
            i[15] = 8; i[16] = 10; i[17] = 11;

            // Fill in the bottom face index data
            i[18] = 12; i[19] = 13; i[20] = 14;
            i[21] = 12; i[22] = 14; i[23] = 15;

            // Fill in the left face index data
            i[24] = 16; i[25] = 17; i[26] = 18;
            i[27] = 16; i[28] = 18; i[29] = 19;

            // Fill in the right face index data
            i[30] = 20; i[31] = 21; i[32] = 22;
            i[33] = 20; i[34] = 22; i[35] = 23;

            AddSubMesh(vertices, i);
        }

        void AddSubMeshSphere(float radius, UINT sliceCount, UINT stackCount)
        {
            std::vector<SimpleMeshVertex> vertices;
            std::vector<std::uint16_t> indices;

            // top
            vertices.emplace_back(0.0f, radius, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);

            float phiStep = DirectX::XM_PI / stackCount;
            float thetaStep = 2.0f * DirectX::XM_PI / sliceCount;

            // Compute vertices for each stack ring (do not count the poles as rings).
            for (UINT i = 1; i <= stackCount - 1; ++i)
            {
                float phi = i * phiStep;

                // Vertices of ring.
                for (UINT j = 0; j <= sliceCount; ++j)
                {
                    float theta = j * thetaStep;

                    SimpleMeshVertex v;

                    // spherical to cartesian
                    v.Position.x = radius * sinf(phi) * cosf(theta);
                    v.Position.y = radius * cosf(phi);
                    v.Position.z = radius * sinf(phi) * sinf(theta);

                    vertices.push_back(v);
                }
            }

            // bottom
            vertices.emplace_back(0.0f, -radius, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);

            //
            // Compute indices for top stack.  The top stack was written first to the vertex buffer
            // and connects the top pole to the first ring.
            //

            for (UINT i = 1; i <= sliceCount; ++i)
            {
                indices.push_back(0);
                indices.push_back(i + 1);
                indices.push_back(i);
            }

            //
            // Compute indices for inner stacks (not connected to poles).
            //

            // Offset the indices to the index of the first vertex in the first ring.
            // This is just skipping the top pole vertex.
            UINT baseIndex = 1;
            UINT ringVertexCount = sliceCount + 1;
            for (UINT i = 0; i < stackCount - 2; ++i)
            {
                for (UINT j = 0; j < sliceCount; ++j)
                {
                    indices.push_back(baseIndex + i * ringVertexCount + j);
                    indices.push_back(baseIndex + i * ringVertexCount + j + 1);
                    indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

                    indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
                    indices.push_back(baseIndex + i * ringVertexCount + j + 1);
                    indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
                }
            }

            //
            // Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
            // and connects the bottom pole to the bottom ring.
            //

            // South pole vertex was added last.
            UINT southPoleIndex = (UINT)vertices.size() - 1;

            // Offset the indices to the index of the first vertex in the last ring.
            baseIndex = southPoleIndex - ringVertexCount;

            for (UINT i = 0; i < sliceCount; ++i)
            {
                indices.push_back(southPoleIndex);
                indices.push_back(baseIndex + i);
                indices.push_back(baseIndex + i + 1);
            }

            AddSubMesh(vertices, indices);
            RecalculateNormals();
        }
    };

    namespace binding
    {
        inline CSHARP_API(SimpleMesh*) SimpleMesh_New()
        {
            return new SimpleMesh();
        }

        inline CSHARP_API(void) SimpleMesh_Delete(SimpleMesh* pObject)
        {
            delete pObject;
        }

        inline CSHARP_API(void) SimpleMesh_ClearSubMeshes(SimpleMesh* pObject)
        {
            pObject->ClearSubMeshes();
        }

        inline CSHARP_API(void) SimpleMesh_AddSubMeshCube(SimpleMesh* pObject)
        {
            pObject->AddSubMeshCube();
        }

        inline CSHARP_API(void) SimpleMesh_AddSubMeshSphere(SimpleMesh* pObject, CSharpFloat radius, CSharpUInt sliceCount, CSharpUInt stackCount)
        {
            pObject->AddSubMeshSphere(radius, sliceCount, stackCount);
        }
    }
}
