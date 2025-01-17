#include "pch.h"
#include "Engine/Graphics/GfxMesh.h"
#include "Engine/Scripting/InteropServices.h"

struct CSharpMeshVertex
{
    cs_vec3 Position;
    cs_vec3 Normal;
    cs_vec4 Tangent;
    cs_vec2 UV;
};

struct CSharpSubMesh
{
    cs_int BaseVertexLocation;
    cs_uint StartIndexLocation;
    cs_uint IndexCount;
};

namespace march
{
    class GfxMeshBinding
    {
    public:
        static inline void SetSubMeshes(cs<GfxMesh*> pObject, cs<CSharpSubMesh[]> subMeshes)
        {
            // 这里只是把 SubMesh 信息清掉，不会清掉顶点信息
            pObject->m_SubMeshes.clear();

            // Vertex Buffer 和 Index Buffer 没变，所以不设置 Dirty

            for (int32_t i = 0; i < subMeshes.size(); i++)
            {
                const CSharpSubMesh& r = subMeshes[i];

                GfxSubMesh& subMesh = pObject->m_SubMeshes.emplace_back();
                subMesh.BaseVertexLocation = r.BaseVertexLocation;
                subMesh.StartIndexLocation = r.StartIndexLocation;
                subMesh.IndexCount = r.IndexCount;
            }
        }

        static inline cs<CSharpMeshVertex[]> GetVertices(cs<GfxMesh*> pObject)
        {
            cs<CSharpMeshVertex[]> results{};
            results.assign(static_cast<int32_t>(pObject->m_Vertices.size()));

            for (int32_t i = 0; i < results.size(); i++)
            {
                const GfxMeshVertex& v = pObject->m_Vertices[i];

                CSharpMeshVertex& r = results[i];
                r.Position.assign(v.Position);
                r.Normal.assign(v.Normal);
                r.Tangent.assign(v.Tangent);
                r.UV.assign(v.UV);
            }

            return results;
        }

        static inline void SetVertices(cs<GfxMesh*> pObject, cs<CSharpMeshVertex[]> vertices)
        {
            pObject->m_Vertices.clear();
            pObject->m_IsDirty = true;

            for (int32_t i = 0; i < vertices.size(); i++)
            {
                const CSharpMeshVertex& v = vertices[i];

                GfxMeshVertex& vertex = pObject->m_Vertices.emplace_back();
                vertex.Position = v.Position;
                vertex.Normal = v.Normal;
                vertex.Tangent = v.Tangent;
                vertex.UV = v.UV;
            }
        }

        static inline cs<cs_ushort[]> GetIndices(cs<GfxMesh*> pObject)
        {
            cs<cs_ushort[]> results{};
            results.assign(static_cast<int32_t>(pObject->m_Indices.size()));

            for (int32_t i = 0; i < results.size(); i++)
            {
                results[i].assign(pObject->m_Indices[i]);
            }

            return results;
        }

        static inline void SetIndices(cs<GfxMesh*> pObject, cs<cs_ushort[]> indices)
        {
            pObject->m_Indices.clear();
            pObject->m_IsDirty = true;

            for (int32_t i = 0; i < indices.size(); i++)
            {
                pObject->m_Indices.push_back(indices[i]);
            }
        }

        static inline void SetBounds(cs<GfxMesh*> pObject, cs_bounds bounds)
        {
            pObject->m_Bounds = bounds;
        }
    };
}

NATIVE_EXPORT_AUTO GfxMesh_New()
{
    retcs MARCH_NEW GfxMesh(GfxAllocator::CommittedDefault);
}

NATIVE_EXPORT_AUTO GfxMesh_GetSubMeshCount(cs<GfxMesh*> pObject)
{
    retcs static_cast<int32_t>(pObject->GetSubMeshCount());
}

NATIVE_EXPORT_AUTO GfxMesh_GetSubMesh(cs<GfxMesh*> pObject, cs_int index)
{
    const GfxSubMesh& subMesh = pObject->GetSubMesh(static_cast<uint32_t>(index.data));

    CSharpSubMesh result = {};
    result.BaseVertexLocation.assign(subMesh.BaseVertexLocation);
    result.StartIndexLocation.assign(subMesh.StartIndexLocation);
    result.IndexCount.assign(subMesh.IndexCount);
    retcs result;
}

NATIVE_EXPORT_AUTO GfxMesh_GetSubMeshes(cs<GfxMesh*> pObject)
{
    cs<CSharpSubMesh[]> results{};
    results.assign(static_cast<int32_t>(pObject->GetSubMeshCount()));

    for (int32_t i = 0; i < results.size(); i++)
    {
        const GfxSubMesh& subMesh = pObject->GetSubMesh(static_cast<uint32_t>(i));

        CSharpSubMesh& r = results[i];
        r.BaseVertexLocation.assign(subMesh.BaseVertexLocation);
        r.StartIndexLocation.assign(subMesh.StartIndexLocation);
        r.IndexCount.assign(subMesh.IndexCount);
    }

    retcs results;
}

NATIVE_EXPORT_AUTO GfxMesh_SetSubMeshes(cs<GfxMesh*> pObject, cs<CSharpSubMesh[]> subMeshes)
{
    GfxMeshBinding::SetSubMeshes(pObject, subMeshes);
}

NATIVE_EXPORT_AUTO GfxMesh_ClearSubMeshes(cs<GfxMesh*> pObject)
{
    pObject->ClearSubMeshes();
}

NATIVE_EXPORT_AUTO GfxMesh_RecalculateNormals(cs<GfxMesh*> pObject)
{
    pObject->RecalculateNormals();
}

NATIVE_EXPORT_AUTO GfxMesh_RecalculateTangents(cs<GfxMesh*> pObject)
{
    pObject->RecalculateTangents();
}

NATIVE_EXPORT_AUTO GfxMesh_AddSubMesh(cs<GfxMesh*> pObject, cs<CSharpMeshVertex[]> vertices, cs<cs_ushort[]> indices)
{
    std::vector<GfxMeshVertex> vertexVec{};
    std::vector<uint16_t> indexVec{};

    for (int32_t i = 0; i < vertices.size(); i++)
    {
        GfxMeshVertex& v = vertexVec.emplace_back();
        v.Position = vertices[i].Position;
        v.Normal = vertices[i].Normal;
        v.Tangent = vertices[i].Tangent;
        v.UV = vertices[i].UV;
    }

    for (int32_t i = 0; i < indices.size(); i++)
    {
        indexVec.push_back(indices[i]);
    }

    pObject->AddSubMesh(static_cast<uint32_t>(vertexVec.size()), vertexVec.data(), static_cast<uint32_t>(indexVec.size()), indexVec.data());
}

NATIVE_EXPORT_AUTO GfxMesh_GetVertices(cs<GfxMesh*> pObject)
{
    retcs GfxMeshBinding::GetVertices(pObject);
}

NATIVE_EXPORT_AUTO GfxMesh_SetVertices(cs<GfxMesh*> pObject, cs<CSharpMeshVertex[]> vertices)
{
    GfxMeshBinding::SetVertices(pObject, vertices);
}

NATIVE_EXPORT_AUTO GfxMesh_GetIndices(cs<GfxMesh*> pObject)
{
    retcs GfxMeshBinding::GetIndices(pObject);
}

NATIVE_EXPORT_AUTO GfxMesh_SetIndices(cs<GfxMesh*> pObject, cs<cs_ushort[]> indices)
{
    GfxMeshBinding::SetIndices(pObject, indices);
}

NATIVE_EXPORT_AUTO GfxMesh_RecalculateBounds(cs<GfxMesh*> pObject)
{
    pObject->RecalculateBounds();
}

NATIVE_EXPORT_AUTO GfxMesh_GetBounds(cs<GfxMesh*> pObject)
{
    retcs pObject->GetBounds();
}

NATIVE_EXPORT_AUTO GfxMesh_SetBounds(cs<GfxMesh*> pObject, cs_bounds bounds)
{
    GfxMeshBinding::SetBounds(pObject, bounds);
}
