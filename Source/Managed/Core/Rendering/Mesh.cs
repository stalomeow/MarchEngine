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
            using NativeArray<MeshVertex> v = vertices;
            using NativeArray<ushort> i = indices;

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

        /// <summary>
        /// 仅用于序列化
        /// </summary>
        [JsonProperty]
        private byte[] CompressedData
        {
            get
            {
                using var subMeshes = (NativeArray<SubMesh>)GfxMesh_GetSubMeshes(NativePtr);
                using var vertices = (NativeArray<MeshVertex>)GfxMesh_GetVertices(NativePtr);
                using var indices = (NativeArray<ushort>)GfxMesh_GetIndices(NativePtr);

                using var ms = new MemoryStream();
                using var writer = new BinaryWriter(ms);

                writer.Write(subMeshes.Length);
                writer.Write(vertices.Length);
                writer.Write(indices.Length);

                for (int i = 0; i < subMeshes.Length; i++)
                {
                    ref SubMesh m = ref subMeshes[i];
                    writer.Write(m.BaseVertexLocation);
                    writer.Write(m.StartIndexLocation);
                    writer.Write(m.IndexCount);
                }

                for (int i = 0; i < vertices.Length; i++)
                {
                    ref MeshVertex v = ref vertices[i];
                    writer.Write(v.Position);
                    writer.Write(v.Normal);
                    writer.Write(v.Tangent);
                    writer.Write(v.UV);
                }

                for (int i = 0; i < indices.Length; i++)
                {
                    writer.Write(indices[i]);
                }

                writer.Flush();
                return ms.ToArray();
            }

            set
            {
                using var reader = new BinaryReader(new MemoryStream(value, writable: false));

                using var subMeshes = new NativeArray<SubMesh>(reader.ReadInt32());
                using var vertices = new NativeArray<MeshVertex>(reader.ReadInt32());
                using var indices = new NativeArray<ushort>(reader.ReadInt32());

                for (int i = 0; i < subMeshes.Length; i++)
                {
                    ref SubMesh m = ref subMeshes[i];
                    m.BaseVertexLocation = reader.ReadInt32();
                    m.StartIndexLocation = reader.ReadUInt32();
                    m.IndexCount = reader.ReadUInt32();
                }

                for (int i = 0; i < vertices.Length; i++)
                {
                    ref MeshVertex v = ref vertices[i];
                    v.Position = reader.ReadVector3();
                    v.Normal = reader.ReadVector3();
                    v.Tangent = reader.ReadVector3();
                    v.UV = reader.ReadVector2();
                }

                for (int i = 0; i < indices.Length; i++)
                {
                    indices[i] = reader.ReadUInt16();
                }

                GfxMesh_SetSubMeshes(NativePtr, subMeshes.Data);
                GfxMesh_SetVertices(NativePtr, vertices.Data);
                GfxMesh_SetIndices(NativePtr, indices.Data);
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
