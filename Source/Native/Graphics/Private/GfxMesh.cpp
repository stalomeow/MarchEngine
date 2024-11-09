#include "GfxMesh.h"
#include "GfxDevice.h"
#include "GfxCommandList.h"
#include "DotNetRuntime.h"
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
            inputs.emplace_back(PipelineInputSematicName::Tangent, 0, DXGI_FORMAT_R32G32B32A32_FLOAT);
            inputs.emplace_back(PipelineInputSematicName::TexCoord, 0, DXGI_FORMAT_R32G32_FLOAT);
            g_PipelineInputDescId = Shader::CreatePipelineInputDesc(inputs, D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        }

        return g_PipelineInputDescId;
    }

    D3D12_PRIMITIVE_TOPOLOGY GfxMesh::GetPrimitiveTopology()
    {
        return Shader::GetPipelineInputDescPrimitiveTopology(GetPipelineInputDescId());
    }

    GfxMesh* GfxMesh::GetGeometry(GfxMeshGeometry geometry)
    {
        return DotNet::RuntimeInvoke<GfxMesh*, GfxMeshGeometry>(ManagedMethod::Mesh_NativeGetGeometry, geometry);
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
            v.Normal = { 0.0f, 0.0f, 0.0f };
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

    void GfxMesh::RecalculateTangents()
    {
        m_IsDirty = true;

        for (auto& v : m_Vertices)
        {
            v.Tangent = { 1.0f, 0.0f, 0.0f, 1.0f };
        }

        for (int i = 0; i < m_Indices.size() / 3; i++)
        {
            auto& v0 = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + 0]];
            auto& v1 = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + 1]];
            auto& v2 = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + 2]];

            XMFLOAT4X4 pos = {};
            pos._11 = v1.Position.x - v0.Position.x;
            pos._12 = v1.Position.y - v0.Position.y;
            pos._13 = v1.Position.z - v0.Position.z;
            pos._21 = v2.Position.x - v0.Position.x;
            pos._22 = v2.Position.y - v0.Position.y;
            pos._23 = v2.Position.z - v0.Position.z;
            pos._33 = 1;
            pos._44 = 1;

            XMFLOAT4X4 uv = {};
            uv._11 = v1.UV.x - v0.UV.x;
            uv._12 = v1.UV.y - v0.UV.y;
            uv._21 = v2.UV.x - v0.UV.x;
            uv._22 = v2.UV.y - v0.UV.y;
            uv._33 = 1;
            uv._44 = 1;

            XMMATRIX result = XMMatrixMultiply(XMMatrixInverse(nullptr, XMLoadFloat4x4(&uv)), XMLoadFloat4x4(&pos));
            XMVECTOR tangent = result.r[0];
            XMVECTOR bitangent = result.r[1];

            for (int j = 0; j < 3; j++)
            {
                auto& v = m_Vertices[m_Indices[static_cast<size_t>(i) * 3 + static_cast<size_t>(j)]];
                XMVECTOR normal = XMLoadFloat3(&v.Normal);

                // 施密特正交化
                XMVECTOR t = XMVector3Normalize(XMVectorSubtract(tangent, XMVectorScale(normal, XMVectorGetX(XMVector3Dot(normal, tangent)))));

                XMVectorSetW(t, XMVectorGetX(XMVector3Dot(XMVector3Cross(normal, t), bitangent)) < 0.0f ? -1.0f : 1.0f);
                XMStoreFloat4(&v.Tangent, t);
            }
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
}
