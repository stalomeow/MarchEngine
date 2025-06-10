#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxMesh.h"
#include "Engine/Rendering/D3D12Impl/GfxPipeline.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Scripting/DotNetMarshal.h"

using namespace DirectX;

namespace march
{
    const GfxInputDesc& GfxMeshVertex::GetInputDesc()
    {
        static const GfxInputDesc inputDesc(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
            {
                GfxInputElement(GfxSemantic::Position, DXGI_FORMAT_R32G32B32_FLOAT),
                GfxInputElement(GfxSemantic::Normal, DXGI_FORMAT_R32G32B32_FLOAT),
                GfxInputElement(GfxSemantic::Tangent, DXGI_FORMAT_R32G32B32A32_FLOAT),
                GfxInputElement(GfxSemantic::TexCoord0, DXGI_FORMAT_R32G32_FLOAT),
            });

        return inputDesc;
    }

    GfxMesh* GfxMesh::GetGeometry(GfxMeshGeometry geometry)
    {
        cs<GfxMeshGeometry> csGeometry{};
        csGeometry.assign(geometry);
        return DotNet::RuntimeInvoke<GfxMesh*>(ManagedMethod::Mesh_NativeGetGeometry, csGeometry);
    }

    GfxMesh::GfxMesh(GfxBufferFlags bufferFlags)
        : GfxBasicMesh(bufferFlags)
        , m_Bounds{}
    {
    }

    void GfxMesh::RecalculateNormals()
    {
        m_IsDirty = true;

        for (GfxMeshVertex& v : m_Vertices)
        {
            v.Normal = { 0.0f, 0.0f, 0.0f };
        }

        for (GfxSubMesh& subMesh : m_SubMeshes)
        {
            size_t baseVertexLocation = static_cast<size_t>(subMesh.BaseVertexLocation);

            for (uint32_t i = 0; i < subMesh.IndexCount / 3; i++)
            {
                size_t indexLocation = static_cast<size_t>(i) * 3 + subMesh.StartIndexLocation;
                GfxMeshVertex& v0 = m_Vertices[baseVertexLocation + m_Indices[indexLocation + 0]];
                GfxMeshVertex& v1 = m_Vertices[baseVertexLocation + m_Indices[indexLocation + 1]];
                GfxMeshVertex& v2 = m_Vertices[baseVertexLocation + m_Indices[indexLocation + 2]];

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
        }

        for (GfxMeshVertex& v : m_Vertices)
        {
            XMStoreFloat3(&v.Normal, XMVector3Normalize(XMLoadFloat3(&v.Normal)));
        }
    }

    void GfxMesh::RecalculateTangents()
    {
        // TODO: 换成 MikkTSpace
        // http://www.mikktspace.com/
        // https://github.com/mmikk/MikkTSpace

        m_IsDirty = true;

        // Ref: https://gamedev.stackexchange.com/questions/68612/how-to-compute-tangent-and-bitangent-vectors

        for (GfxMeshVertex& v : m_Vertices)
        {
            v.Tangent = { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        std::vector<XMFLOAT3> bitangents(m_Vertices.size(), { 0.0f, 0.0f, 0.0f });

        for (GfxSubMesh& subMesh : m_SubMeshes)
        {
            size_t baseVertexLocation = static_cast<size_t>(subMesh.BaseVertexLocation);

            for (uint32_t i = 0; i < subMesh.IndexCount / 3; i++)
            {
                size_t indexLocation = static_cast<size_t>(i) * 3 + subMesh.StartIndexLocation;
                size_t i0 = baseVertexLocation + m_Indices[indexLocation + 0];
                size_t i1 = baseVertexLocation + m_Indices[indexLocation + 1];
                size_t i2 = baseVertexLocation + m_Indices[indexLocation + 2];

                GfxMeshVertex& v0 = m_Vertices[i0];
                GfxMeshVertex& v1 = m_Vertices[i1];
                GfxMeshVertex& v2 = m_Vertices[i2];

                float dx1 = v1.Position.x - v0.Position.x;
                float dy1 = v1.Position.y - v0.Position.y;
                float dz1 = v1.Position.z - v0.Position.z;
                float dx2 = v2.Position.x - v0.Position.x;
                float dy2 = v2.Position.y - v0.Position.y;
                float dz2 = v2.Position.z - v0.Position.z;

                float du1 = v1.UV.x - v0.UV.x;
                float dv1 = v1.UV.y - v0.UV.y;
                float du2 = v2.UV.x - v0.UV.x;
                float dv2 = v2.UV.y - v0.UV.y;
                float duvDetInv = 1.0f / (du1 * dv2 - du2 * dv1);

                float tx = (dv2 * dx1 - dv1 * dx2) * duvDetInv;
                float ty = (dv2 * dy1 - dv1 * dy2) * duvDetInv;
                float tz = (dv2 * dz1 - dv1 * dz2) * duvDetInv;
                float bx = (du1 * dx2 - du2 * dx1) * duvDetInv;
                float by = (du1 * dy2 - du2 * dy1) * duvDetInv;
                float bz = (du1 * dz2 - du2 * dz1) * duvDetInv;

                v0.Tangent.x += tx;
                v0.Tangent.y += ty;
                v0.Tangent.z += tz;
                v1.Tangent.x += tx;
                v1.Tangent.y += ty;
                v1.Tangent.z += tz;
                v2.Tangent.x += tx;
                v2.Tangent.y += ty;
                v2.Tangent.z += tz;

                bitangents[i0].x += bx;
                bitangents[i0].y += by;
                bitangents[i0].z += bz;
                bitangents[i1].x += bx;
                bitangents[i1].y += by;
                bitangents[i1].z += bz;
                bitangents[i2].x += bx;
                bitangents[i2].y += by;
                bitangents[i2].z += bz;
            }
        }

        for (size_t i = 0; i < m_Vertices.size(); i++)
        {
            GfxMeshVertex& v = m_Vertices[i];
            XMVECTOR normal = XMLoadFloat3(&v.Normal);
            XMVECTOR tangent = XMLoadFloat4(&v.Tangent);
            XMVECTOR bitangent = XMLoadFloat3(&bitangents[i]);

            XMVECTOR t = XMVector3Normalize(XMVectorSubtract(tangent, XMVectorScale(normal, XMVectorGetX(XMVector3Dot(normal, tangent)))));

            XMVectorSetW(t, XMVectorGetX(XMVector3Dot(XMVector3Cross(normal, t), bitangent)) < 0.0f ? -1.0f : 1.0f);
            XMStoreFloat4(&v.Tangent, t);
        }
    }

    void GfxMesh::RecalculateBounds()
    {
        BoundingBox::CreateFromPoints(m_Bounds, m_Vertices.size(), &m_Vertices.data()->Position, sizeof(GfxMeshVertex));
    }
}
