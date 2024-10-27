using March.Core.Binding;
using Newtonsoft.Json;
using System.Numerics;
using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    [StructLayout(LayoutKind.Sequential)]
    public struct MeshVertex(Vector3 position, Vector3 normal, Vector3 tangent, Vector2 uv)
    {
        public Vector3 Position = position;
        public Vector3 Normal = normal;
        public Vector3 Tangent = tangent;
        public Vector2 UV = uv;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SubMesh
    {
        public int BaseVertexLocation;
        public uint StartIndexLocation;
        public uint IndexCount;
    }

    public partial class Mesh : NativeMarchObject
    {
        public Mesh() : base(GfxMesh_New()) { }

        protected override void Dispose(bool disposing)
        {
            GfxMesh_Delete(NativePtr);
        }

        public int SubMeshCount => GfxMesh_GetSubMeshCount(NativePtr);

        public SubMesh GetSubMesh(int index)
        {
            return GfxMesh_GetSubMesh(NativePtr, index);
        }

        public void ClearSubMeshes()
        {
            GfxMesh_ClearSubMeshes(NativePtr);
        }

        public void RecalculateNormals()
        {
            GfxMesh_RecalculateNormals(NativePtr);
        }

        public void AddSubMesh(List<MeshVertex> vertices, List<ushort> indices)
        {
            using NativeArray<MeshVertex> v = CollectionsMarshal.AsSpan(vertices);
            using NativeArray<ushort> i = CollectionsMarshal.AsSpan(indices);

            GfxMesh_AddSubMesh(NativePtr, v.Data, i.Data);
        }

        public void AddSubMeshCube()
        {
            GfxMesh_AddSubMeshCube(NativePtr);
        }

        public void AddSubMeshSphere(float radius, uint sliceCount, uint stackCount)
        {
            GfxMesh_AddSubMeshSphere(NativePtr, radius, sliceCount, stackCount);
        }

        #region Serialization

        [JsonProperty("SubMeshes")]
        private SubMesh[] SubMeshesSerializationOnly
        {
            get
            {
                nint subMeshes = GfxMesh_GetSubMeshes(NativePtr);
                return NativeArray<SubMesh>.GetAndFree(subMeshes);
            }

            set
            {
                using NativeArray<SubMesh> v = value;
                GfxMesh_SetSubMeshes(NativePtr, v.Data);
            }
        }

        [JsonProperty("Vertices")]
        private MeshVertex[] VerticesSerializationOnly
        {
            get
            {
                nint vertices = GfxMesh_GetVertices(NativePtr);
                return NativeArray<MeshVertex>.GetAndFree(vertices);
            }

            set
            {
                using NativeArray<MeshVertex> v = value;
                GfxMesh_SetVertices(NativePtr, v.Data);
            }
        }

        [JsonProperty("Indices")]
        private ushort[] IndicesSerializationOnly
        {
            get
            {
                nint indices = GfxMesh_GetIndices(NativePtr);
                return NativeArray<ushort>.GetAndFree(indices);
            }

            set
            {
                using NativeArray<ushort> v = value;
                GfxMesh_SetIndices(NativePtr, v.Data);
            }
        }

        #endregion

        #region Bindings

        [NativeFunction]
        private static partial nint GfxMesh_New();

        [NativeFunction]
        private static partial void GfxMesh_Delete(nint self);

        [NativeFunction]
        private static partial int GfxMesh_GetSubMeshCount(nint self);

        [NativeFunction]
        private static partial SubMesh GfxMesh_GetSubMesh(nint self, int index);

        [NativeFunction]
        private static partial nint GfxMesh_GetSubMeshes(nint self);

        [NativeFunction]
        private static partial void GfxMesh_SetSubMeshes(nint self, nint subMeshes);

        [NativeFunction]
        private static partial void GfxMesh_ClearSubMeshes(nint self);

        [NativeFunction]
        private static partial void GfxMesh_RecalculateNormals(nint self);

        [NativeFunction]
        private static partial void GfxMesh_AddSubMesh(nint self, nint vertices, nint indices);

        [NativeFunction]
        private static partial void GfxMesh_AddSubMeshCube(nint self);

        [NativeFunction]
        private static partial void GfxMesh_AddSubMeshSphere(nint self, float radius, uint sliceCount, uint stackCount);

        [NativeFunction]
        private static partial nint GfxMesh_GetVertices(nint self);

        [NativeFunction]
        private static partial void GfxMesh_SetVertices(nint self, nint vertices);

        [NativeFunction]
        private static partial nint GfxMesh_GetIndices(nint self);

        [NativeFunction]
        private static partial void GfxMesh_SetIndices(nint self, nint indices);

        #endregion
    }
}
