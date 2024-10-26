#include "GfxMesh.h"
#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "Shader.h"
#include <Windows.h>

using namespace DirectX;

namespace march
{
    static int32_t g_PipelineInputDescId = Shader::GetInvalidPipelineInputDescId();

    int32_t GfxMesh::GetPipelineInputDescId()
    {
        if (g_PipelineInputDescId == Shader::GetInvalidPipelineInputDescId())
        {
            std::vector<PipelineInputElement> inputs{};
            inputs.emplace_back(PipelineInputSematicName::Position, 0, DXGI_FORMAT_R32G32B32_FLOAT);
            inputs.emplace_back(PipelineInputSematicName::Normal, 0, DXGI_FORMAT_R32G32B32_FLOAT);
            inputs.emplace_back(PipelineInputSematicName::Tangent, 0, DXGI_FORMAT_R32G32B32_FLOAT);
            inputs.emplace_back(PipelineInputSematicName::TexCoord, 0, DXGI_FORMAT_R32G32_FLOAT);
            g_PipelineInputDescId = Shader::CreatePipelineInputDesc(inputs, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }

        return g_PipelineInputDescId;
    }

    D3D12_PRIMITIVE_TOPOLOGY GfxMesh::GetPrimitiveTopology()
    {
        return Shader::GetPipelineInputDescPrimitiveTopology(GetPipelineInputDescId());
    }

    GfxMesh::GfxMesh()
        : m_SubMeshes{}
        , m_Vertices{}
        , m_Indices{}
        , m_IsDirty(false)
        , m_VertexBuffer(nullptr)
        , m_IndexBuffer(nullptr)
    {
    }

    uint32_t GfxMesh::GetSubMeshCount() const
    {
        return static_cast<uint32_t>(m_SubMeshes.size());
    }

    const GfxSubMesh& GfxMesh::GetSubMesh(uint32_t index) const
    {
        return m_SubMeshes[index];
    }

    void GfxMesh::ClearSubMeshes()
    {
        if (!m_SubMeshes.empty())
        {
            m_IsDirty = true;
        }

        m_SubMeshes.clear();
        m_Vertices.clear();
        m_Indices.clear();
    }

    static void UploadToBuffer(GfxBuffer* dest, const void* pData, uint32_t size)
    {
        D3D12_SUBRESOURCE_DATA subResData = {};
        subResData.pData = pData;
        subResData.RowPitch = static_cast<LONG_PTR>(size);
        subResData.SlicePitch = static_cast<LONG_PTR>(size);

        GfxDevice* device = GetGfxDevice();
        GfxUploadMemory m = device->AllocateTransientUploadMemory(size);
        GfxCommandList* cmdList = device->GetGraphicsCommandList();

        cmdList->ResourceBarrier(dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
        UpdateSubresources(cmdList->GetD3D12CommandList(), dest->GetD3D12Resource(),
            m.GetD3D12Resource(), static_cast<UINT64>(m.GetD3D12ResourceOffset(0)), 0, 1, &subResData);
        cmdList->ResourceBarrier(dest, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    void GfxMesh::GetBufferViews(D3D12_VERTEX_BUFFER_VIEW& vbv, D3D12_INDEX_BUFFER_VIEW& ibv)
    {
        GfxDevice* device = GetGfxDevice();
        GfxCommandList* cmdList = device->GetGraphicsCommandList();

        if (m_IsDirty)
        {
            m_VertexBuffer = std::make_unique<GfxVertexBuffer<GfxMeshVertex>>(device, "MeshVertexBuffer", static_cast<uint32_t>(m_Vertices.size()));
            UploadToBuffer(m_VertexBuffer.get(), m_Vertices.data(), m_VertexBuffer->GetSize());

            m_IndexBuffer = std::make_unique<GfxIndexBuffer<uint16_t>>(device, "MeshIndexBuffer", static_cast<uint32_t>(m_Indices.size()));
            UploadToBuffer(m_IndexBuffer.get(), m_Indices.data(), m_IndexBuffer->GetSize());

            cmdList->FlushResourceBarriers();
            m_IsDirty = false;
        }

        vbv = m_VertexBuffer->GetView();
        ibv = m_IndexBuffer->GetView();
    }

    void GfxMesh::RecalculateNormals()
    {
        m_IsDirty = true;

        for (auto& v : m_Vertices)
        {
            v.Normal = { 0.0, 0.0, 0.0 };
        }

        for (int i = 0; i < m_Indices.size() / 3; i++)
        {
            auto& v0 = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + 0]];
            auto& v1 = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + 1]];
            auto& v2 = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + 2]];

            XMVECTOR p0 = XMLoadFloat3(&v0.Position);
            XMVECTOR p1 = XMLoadFloat3(&v1.Position);
            XMVECTOR p2 = XMLoadFloat3(&v2.Position);
            XMVECTOR vec1 = XMVectorSubtract(p1, p0);
            XMVECTOR vec2 = XMVectorSubtract(p2, p0);
            XMVECTOR normal = XMVector3Normalize(XMVector3Cross(vec1, vec2));

            XMStoreFloat3(&v0.Normal, XMVectorAdd(XMLoadFloat3(&v0.Normal), normal));
            XMStoreFloat3(&v1.Normal, XMVectorAdd(XMLoadFloat3(&v1.Normal), normal));
            XMStoreFloat3(&v2.Normal, XMVectorAdd(XMLoadFloat3(&v2.Normal), normal));
        }

        for (auto& v : m_Vertices)
        {
            XMStoreFloat3(&v.Normal, XMVector3Normalize(XMLoadFloat3(&v.Normal)));
        }
    }

    void GfxMesh::AddSubMesh(const std::vector<GfxMeshVertex>& vertices, const std::vector<uint16_t>& indices)
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

    void GfxMesh::AddSubMeshCube(float width, float height, float depth)
    {
        std::vector<GfxMeshVertex> vertices;
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

    void GfxMesh::AddSubMeshSphere(float radius, uint32_t sliceCount, uint32_t stackCount)
    {
        std::vector<GfxMeshVertex> vertices;
        std::vector<std::uint16_t> indices;

        // top
        vertices.emplace_back(0.0f, radius, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

        float phiStep = XM_PI / stackCount;
        float thetaStep = 2.0f * XM_PI / sliceCount;

        // Compute vertices for each stack ring (do not count the poles as rings).
        for (UINT i = 1; i <= stackCount - 1; ++i)
        {
            float phi = i * phiStep;

            // Vertices of ring.
            for (UINT j = 0; j <= sliceCount; ++j)
            {
                float theta = j * thetaStep;

                GfxMeshVertex v;

                // spherical to cartesian
                v.Position.x = radius * sinf(phi) * cosf(theta);
                v.Position.y = radius * cosf(phi);
                v.Position.z = radius * sinf(phi) * sinf(theta);

                vertices.push_back(v);
            }
        }

        // bottom
        vertices.emplace_back(0.0f, -radius, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

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

    void GfxMesh::AddFullScreenTriangle()
    {
        std::vector<GfxMeshVertex> vertices;
        std::vector<std::uint16_t> indices;

        vertices.emplace_back();
        vertices.emplace_back();
        vertices.emplace_back();

        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        AddSubMesh(vertices, indices);
    }
}
