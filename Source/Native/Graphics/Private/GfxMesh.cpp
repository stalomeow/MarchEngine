#include "GfxMesh.h"
#include "GfxDevice.h"
#include "GfxBuffer.h"
#include "GfxCommandList.h"
#include <vector>
#include <wrl.h>
#include <assert.h>
#include <Windows.h>
#include <memory>
#include <DirectXMath.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace march
{
    GfxMesh::GfxMesh(GfxDevice* device) : m_Device(device)
    {
    }

    template<typename TVertex, typename TIndex>
    class GfxMeshImpl : public GfxMesh
    {
        static_assert(sizeof(TIndex) == 2 || sizeof(TIndex) == 4, "TIndex must be 2 or 4 bytes in size.");

    public:
        GfxMeshImpl(GfxDevice* device, D3D12_PRIMITIVE_TOPOLOGY topology);
        virtual ~GfxMeshImpl() = default;

        void Draw() override;
        void Draw(uint32_t subMeshIndex) override;
        void RecalculateNormals() override;

        void ClearSubMeshes() override;
        void AddSubMesh(const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices);

        uint32_t GetSubMeshCount() const override { return static_cast<uint32_t>(m_SubMeshes.size()); }
        D3D12_PRIMITIVE_TOPOLOGY GetTopology() const override { return m_Topology; }

        D3D12_PRIMITIVE_TOPOLOGY_TYPE GetTopologyType() const override;
        D3D12_INPUT_LAYOUT_DESC GetVertexInputLayout() const override;

    protected:
        void UpdateBufferIfDirty();
        static void UploadToBuffer(GfxBuffer* dest, const void* pData, uint32_t size);

    private:
        D3D12_PRIMITIVE_TOPOLOGY m_Topology;
        std::vector<GfxSubMesh> m_SubMeshes;
        std::vector<TVertex> m_Vertices;
        std::vector<TIndex> m_Indices;
        bool m_IsDirty;

        std::unique_ptr<GfxVertexBuffer<TVertex>> m_VertexBuffer;
        std::unique_ptr<GfxIndexBuffer<TIndex>> m_IndexBuffer;
    };

    template<typename TVertex, typename TIndex>
    GfxMeshImpl<TVertex, TIndex>::GfxMeshImpl(GfxDevice* device, D3D12_PRIMITIVE_TOPOLOGY topology)
        : GfxMesh(device)
        , m_Topology(topology), m_SubMeshes{}, m_Vertices{}, m_Indices{}, m_IsDirty(false)
        , m_VertexBuffer(nullptr), m_IndexBuffer(nullptr)
    {
    }

    template<typename TVertex, typename TIndex>
    void GfxMeshImpl<TVertex, TIndex>::Draw()
    {
        UpdateBufferIfDirty();

        GfxCommandList* cmdList = GetDevice()->GetGraphicsCommandList();
        cmdList->GetD3D12CommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer->GetView());
        cmdList->GetD3D12CommandList()->IASetIndexBuffer(&m_IndexBuffer->GetView());
        cmdList->GetD3D12CommandList()->IASetPrimitiveTopology(m_Topology);

        cmdList->FlushResourceBarriers();

        for (GfxSubMesh& subMesh : m_SubMeshes)
        {
            cmdList->GetD3D12CommandList()->DrawIndexedInstanced(static_cast<UINT>(subMesh.IndexCount), 1,
                static_cast<UINT>(subMesh.StartIndexLocation), static_cast<INT>(subMesh.BaseVertexLocation), 0);
        }
    }

    template<typename TVertex, typename TIndex>
    void GfxMeshImpl<TVertex, TIndex>::Draw(uint32_t subMeshIndex)
    {
        UpdateBufferIfDirty();

        GfxCommandList* cmdList = GetDevice()->GetGraphicsCommandList();
        cmdList->GetD3D12CommandList()->IASetVertexBuffers(0, 1, &m_VertexBuffer->GetView());
        cmdList->GetD3D12CommandList()->IASetIndexBuffer(&m_IndexBuffer->GetView());
        cmdList->GetD3D12CommandList()->IASetPrimitiveTopology(m_Topology);

        cmdList->FlushResourceBarriers();

        GfxSubMesh& subMesh = m_SubMeshes[static_cast<size_t>(subMeshIndex)];
        cmdList->GetD3D12CommandList()->DrawIndexedInstanced(static_cast<UINT>(subMesh.IndexCount), 1,
            static_cast<UINT>(subMesh.StartIndexLocation), static_cast<INT>(subMesh.BaseVertexLocation), 0);
    }

    template<typename TVertex, typename TIndex>
    void GfxMeshImpl<TVertex, TIndex>::UpdateBufferIfDirty()
    {
        if (!m_IsDirty)
        {
            return;
        }

        m_VertexBuffer = std::make_unique<GfxVertexBuffer<TVertex>>(GetDevice(), "MeshVertexBuffer", static_cast<uint32_t>(m_Vertices.size()));
        UploadToBuffer(m_VertexBuffer.get(), m_Vertices.data(), m_VertexBuffer->GetSize());

        m_IndexBuffer = std::make_unique<GfxIndexBuffer<TIndex>>(GetDevice(), "MeshIndexBuffer", static_cast<uint32_t>(m_Indices.size()));
        UploadToBuffer(m_IndexBuffer.get(), m_Indices.data(), m_IndexBuffer->GetSize());

        m_IsDirty = false;
    }

    template<typename TVertex, typename TIndex>
    void GfxMeshImpl<TVertex, TIndex>::UploadToBuffer(GfxBuffer* dest, const void* pData, uint32_t size)
    {
        D3D12_SUBRESOURCE_DATA subResData = {};
        subResData.pData = pData;
        subResData.RowPitch = static_cast<LONG_PTR>(size);
        subResData.SlicePitch = static_cast<LONG_PTR>(size);

        // TODO remove static
        GfxUploadMemory m = GetGfxDevice()->AllocateTransientUploadMemory(size);
        GfxCommandList* cmdList = GetGfxDevice()->GetGraphicsCommandList();

        cmdList->ResourceBarrier(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
        UpdateSubresources(cmdList->GetD3D12CommandList(), dest->GetD3D12Resource(),
            m.GetD3D12Resource(), static_cast<UINT64>(m.GetD3D12ResourceOffset(0)), 0, 1, &subResData);
        cmdList->ResourceBarrier(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    template<typename TVertex, typename TIndex>
    void GfxMeshImpl<TVertex, TIndex>::RecalculateNormals()
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
    void GfxMeshImpl<TVertex, TIndex>::ClearSubMeshes()
    {
        if (!m_SubMeshes.empty())
        {
            m_IsDirty = true;
        }

        m_SubMeshes.clear();
        m_Vertices.clear();
        m_Indices.clear();
    }

    template<typename TVertex, typename TIndex>
    void GfxMeshImpl<TVertex, TIndex>::AddSubMesh(const std::vector<TVertex>& vertices, const std::vector<TIndex>& indices)
    {
        GfxSubMesh subMesh = {};
        subMesh.BaseVertexLocation = static_cast<int32_t>(m_Vertices.size());
        subMesh.IndexCount = static_cast<uint32_t>(indices.size());
        subMesh.StartIndexLocation = static_cast<uint32_t>(m_Indices.size());

        m_IsDirty = true;
        m_SubMeshes.push_back(subMesh);
        m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
        m_Indices.insert(m_Indices.end(), indices.begin(), indices.end());
    }

    template<typename TVertex, typename TIndex>
    D3D12_PRIMITIVE_TOPOLOGY_TYPE GfxMeshImpl<TVertex, TIndex>::GetTopologyType() const
    {
        switch (m_Topology)
        {
        case D3D_PRIMITIVE_TOPOLOGY_UNDEFINED:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;

        case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

        case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
        case D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ:
        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
        case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ:
        case D3D_PRIMITIVE_TOPOLOGY_TRIANGLEFAN:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        }
    }

    template<typename TVertex, typename TIndex>
    D3D12_INPUT_LAYOUT_DESC GfxMeshImpl<TVertex, TIndex>::GetVertexInputLayout() const
    {
        D3D12_INPUT_LAYOUT_DESC desc = {};
        desc.pInputElementDescs = TVertex::InputDesc;
        desc.NumElements = static_cast<UINT>(std::size(TVertex::InputDesc));
        return desc;
    }

    struct SimpleMeshVertex
    {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT3 Tangent;
        XMFLOAT2 UV;

        static constexpr D3D12_INPUT_ELEMENT_DESC InputDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };

        SimpleMeshVertex() = default;

        SimpleMeshVertex(float x, float y, float z, float nx, float ny, float nz, float tx, float ty, float tz, float u, float v)
            : Position(x, y, z), Normal(nx, ny, nz), Tangent(tx, ty, tz), UV(u, v) {}
    };

    class GfxSimpleMesh : public GfxMeshImpl<SimpleMeshVertex, std::uint16_t>
    {
    public:
        GfxSimpleMesh(GfxDevice* device);
        virtual ~GfxSimpleMesh() = default;

        void AddSubMeshCube(float width, float height, float depth) override;
        void AddSubMeshSphere(float radius, uint32_t sliceCount, uint32_t stackCount) override;
    };

    GfxSimpleMesh::GfxSimpleMesh(GfxDevice* device)
        : GfxMeshImpl<SimpleMeshVertex, std::uint16_t>(device, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
    {
    }

    void GfxSimpleMesh::AddSubMeshCube(float width, float height, float depth)
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

    void GfxSimpleMesh::AddSubMeshSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
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

    GfxMesh* CreateSimpleGfxMesh(GfxDevice* device)
    {
        return new GfxSimpleMesh(device);
    }

    void ReleaseSimpleGfxMesh(GfxMesh* mesh)
    {
        delete mesh;
    }
}
