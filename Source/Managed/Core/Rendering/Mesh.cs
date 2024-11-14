using March.Core.Interop;
using Newtonsoft.Json;
using System.Numerics;
using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    [StructLayout(LayoutKind.Sequential)]
    public struct MeshVertex(Vector3 position, Vector3 normal, Vector4 tangent, Vector2 uv)
    {
        public Vector3 Position = position;
        public Vector3 Normal = normal;
        public Vector4 Tangent = tangent;
        public Vector2 UV = uv;

        public MeshVertex(Vector3 position, Vector2 uv) : this(position, Vector3.Zero, Vector4.Zero, uv) { }

        public MeshVertex(float x, float y, float z, float u, float v) : this(new Vector3(x, y, z), new Vector2(u, v)) { }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SubMesh
    {
        public int BaseVertexLocation;
        public uint StartIndexLocation;
        public uint IndexCount;
    }

    [NativeTypeName("GfxMesh")]
    public partial class Mesh : NativeMarchObject
    {
        public Mesh() : base(New()) { }

        protected override void Dispose(bool disposing)
        {
            Delete();
        }

        [NativeProperty]
        public partial int SubMeshCount { get; }

        [NativeMethod]
        public partial SubMesh GetSubMesh(int index);

        [NativeMethod]
        public partial void ClearSubMeshes();

        [NativeProperty("Bounds")]
        public partial Bounds bounds { get; private set; }

        public void AddSubMesh(List<MeshVertex> vertices, List<ushort> indices)
        {
            AddSubMesh(CollectionsMarshal.AsSpan(vertices), CollectionsMarshal.AsSpan(indices));
        }

        public void AddSubMesh(ReadOnlySpan<MeshVertex> vertices, ReadOnlySpan<ushort> indices)
        {
            using NativeArray<MeshVertex> v = vertices;
            using NativeArray<ushort> i = indices;

            AddSubMesh(v.Data, i.Data);
        }

        [NativeMethod]
        public partial void RecalculateNormals();

        /// <summary>
        /// 注意：计算 Tangent 需要使用 Normal
        /// </summary>
        [NativeMethod]
        public partial void RecalculateTangents();

        [NativeMethod]
        public partial void RecalculateBounds();

        #region Serialization

        /// <summary>
        /// 仅用于序列化
        /// </summary>
        [JsonProperty]
        private byte[] CompressedData
        {
            get
            {
                using var subMeshes = (NativeArray<SubMesh>)SubMeshes;
                using var vertices = (NativeArray<MeshVertex>)Vertices;
                using var indices = (NativeArray<ushort>)Indices;

                using var ms = new MemoryStream();
                using var writer = new BinaryWriter(ms);

                writer.Write(bounds);
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

                bounds = reader.ReadBounds();

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
                    v.Tangent = reader.ReadVector4();
                    v.UV = reader.ReadVector2();
                }

                for (int i = 0; i < indices.Length; i++)
                {
                    indices[i] = reader.ReadUInt16();
                }

                SubMeshes = subMeshes.Data;
                Vertices = vertices.Data;
                Indices = indices.Data;
            }
        }

        #endregion

        [NativeProperty]
        private partial nint SubMeshes { get; set; }

        [NativeProperty]
        private partial nint Vertices { get; set; }

        [NativeProperty]
        private partial nint Indices { get; set; }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void Delete();

        [NativeMethod]
        private partial void AddSubMesh(nint vertices, nint indices);
    }
}
