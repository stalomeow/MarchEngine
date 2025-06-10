using March.Core.Pool;
using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    public enum MeshGeometry
    {
        FullScreenTriangle,
        Cube,
        Sphere,
    }

    public partial class Mesh
    {
        private static readonly Dictionary<MeshGeometry, Mesh> s_Geometries = new();

        internal static void InitializeGeometries()
        {
            Application.OnQuit += () =>
            {
                foreach (KeyValuePair<MeshGeometry, Mesh> kv in s_Geometries)
                {
                    kv.Value.Dispose();
                }

                s_Geometries.Clear();
            };
        }

        public static Mesh GetGeometry(MeshGeometry geometry)
        {
            if (!s_Geometries.TryGetValue(geometry, out Mesh? mesh))
            {
                mesh = new Mesh();
                mesh.AddDefaultGeometry(geometry);
                s_Geometries.Add(geometry, mesh);
            }

            return mesh;
        }

        [UnmanagedCallersOnly]
        private static nint NativeGetGeometry(MeshGeometry geometry)
        {
            return GetGeometry(geometry).NativePtr;
        }

        private void AddDefaultGeometry(MeshGeometry geometry)
        {
            switch (geometry)
            {
                case MeshGeometry.FullScreenTriangle:
                    AddFullScreenTriangle();
                    break;

                case MeshGeometry.Cube:
                    AddSubMeshCube(1.0f, 1.0f, 1.0f);
                    break;

                case MeshGeometry.Sphere:
                    AddSubMeshSphere(0.5f, 60, 60);
                    break;

                default:
                    throw new NotSupportedException("Unsupported mesh geometry type: " + geometry.ToString());
            }
        }

        private void AddFullScreenTriangle()
        {
            Span<MeshVertex> vertices = [default, default, default];
            Span<ushort> indices = [0, 1, 2];
            AddSubMesh(vertices, indices);
        }

        private void AddSubMeshCube(float width, float height, float depth)
        {
            float w2 = 0.5f * width;
            float h2 = 0.5f * height;
            float d2 = 0.5f * depth;

            Span<MeshVertex> vertices =
            [
                // Fill in the front face vertex data.
                new MeshVertex(-w2, -h2, -d2, 0.0f, 1.0f),
                new MeshVertex(-w2, +h2, -d2, 0.0f, 0.0f),
                new MeshVertex(+w2, +h2, -d2, 1.0f, 0.0f),
                new MeshVertex(+w2, -h2, -d2, 1.0f, 1.0f),

                // Fill in the back face vertex data.
                new MeshVertex(-w2, -h2, +d2, 1.0f, 1.0f),
                new MeshVertex(+w2, -h2, +d2, 0.0f, 1.0f),
                new MeshVertex(+w2, +h2, +d2, 0.0f, 0.0f),
                new MeshVertex(-w2, +h2, +d2, 1.0f, 0.0f),

                // Fill in the top face vertex data.
                new MeshVertex(-w2, +h2, -d2, 0.0f, 1.0f),
                new MeshVertex(-w2, +h2, +d2, 0.0f, 0.0f),
                new MeshVertex(+w2, +h2, +d2, 1.0f, 0.0f),
                new MeshVertex(+w2, +h2, -d2, 1.0f, 1.0f),

                // Fill in the bottom face vertex data.
                new MeshVertex(-w2, -h2, -d2, 1.0f, 1.0f),
                new MeshVertex(+w2, -h2, -d2, 0.0f, 1.0f),
                new MeshVertex(+w2, -h2, +d2, 0.0f, 0.0f),
                new MeshVertex(-w2, -h2, +d2, 1.0f, 0.0f),

                // Fill in the left face vertex data.
                new MeshVertex(-w2, -h2, +d2, 0.0f, 1.0f),
                new MeshVertex(-w2, +h2, +d2, 0.0f, 0.0f),
                new MeshVertex(-w2, +h2, -d2, 1.0f, 0.0f),
                new MeshVertex(-w2, -h2, -d2, 1.0f, 1.0f),

                // Fill in the right face vertex data.
                new MeshVertex(+w2, -h2, -d2, 0.0f, 1.0f),
                new MeshVertex(+w2, +h2, -d2, 0.0f, 0.0f),
                new MeshVertex(+w2, +h2, +d2, 1.0f, 0.0f),
                new MeshVertex(+w2, -h2, +d2, 1.0f, 1.0f),
            ];

            Span<ushort> indices =
            [
                // Fill in the front face index data
                0, 1, 2, 0, 2, 3,

                // Fill in the back face index data
                4, 5, 6, 4, 6, 7,

                // Fill in the top face index data
                8, 9, 10, 8, 10, 11,

                // Fill in the bottom face index data
                12, 13, 14, 12, 14, 15,

                // Fill in the left face index data
                16, 17, 18, 16, 18, 19,

                // Fill in the right face index data
                20, 21, 22, 20, 22, 23,
            ];

            AddSubMesh(vertices, indices);
            RecalculateNormals();
            RecalculateTangents();
            RecalculateBounds();
        }

        private void AddSubMeshSphere(float radius, int sliceCount, int stackCount)
        {
            using var vertices = ListPool<MeshVertex>.Get();
            using var indices = ListPool<ushort>.Get();

            // top
            vertices.Value.Add(new MeshVertex(0.0f, radius, 0.0f, 0.0f, 0.0f));

            float phiStep = MathF.PI / stackCount;
            float thetaStep = 2.0f * MathF.PI / sliceCount;

            // Compute vertices for each stack ring (do not count the poles as rings).
            for (int i = 1; i <= stackCount - 1; i++)
            {
                float phi = i * phiStep;

                // Vertices of ring.
                for (int j = 0; j <= sliceCount; j++)
                {
                    float theta = j * thetaStep;

                    // spherical to cartesian
                    float x = radius * MathF.Sin(phi) * MathF.Cos(theta);
                    float y = radius * MathF.Cos(phi);
                    float z = radius * MathF.Sin(phi) * MathF.Sin(theta);

                    float u = theta / (2.0f * MathF.PI);
                    float v = phi / MathF.PI;

                    vertices.Value.Add(new MeshVertex(x, y, z, u, v));
                }
            }

            // bottom
            vertices.Value.Add(new MeshVertex(0.0f, -radius, 0.0f, 0.0f, 1.0f));

            //
            // Compute indices for top stack.  The top stack was written first to the vertex buffer
            // and connects the top pole to the first ring.
            //

            for (int i = 1; i <= sliceCount; ++i)
            {
                indices.Value.Add(0);
                indices.Value.Add((ushort)(i + 1));
                indices.Value.Add((ushort)i);
            }

            //
            // Compute indices for inner stacks (not connected to poles).
            //

            // Offset the indices to the index of the first vertex in the first ring.
            // This is just skipping the top pole vertex.
            int baseIndex = 1;
            int ringVertexCount = sliceCount + 1;
            for (int i = 0; i < stackCount - 2; ++i)
            {
                for (int j = 0; j < sliceCount; ++j)
                {
                    indices.Value.Add((ushort)(baseIndex + i * ringVertexCount + j));
                    indices.Value.Add((ushort)(baseIndex + i * ringVertexCount + j + 1));
                    indices.Value.Add((ushort)(baseIndex + (i + 1) * ringVertexCount + j));

                    indices.Value.Add((ushort)(baseIndex + (i + 1) * ringVertexCount + j));
                    indices.Value.Add((ushort)(baseIndex + i * ringVertexCount + j + 1));
                    indices.Value.Add((ushort)(baseIndex + (i + 1) * ringVertexCount + j + 1));
                }
            }

            //
            // Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
            // and connects the bottom pole to the bottom ring.
            //

            // South pole vertex was added last.
            int southPoleIndex = vertices.Value.Count - 1;

            // Offset the indices to the index of the first vertex in the last ring.
            baseIndex = southPoleIndex - ringVertexCount;

            for (int i = 0; i < sliceCount; ++i)
            {
                indices.Value.Add((ushort)southPoleIndex);
                indices.Value.Add((ushort)(baseIndex + i));
                indices.Value.Add((ushort)(baseIndex + i + 1));
            }

            AddSubMesh(vertices, indices);
            RecalculateNormals();
            RecalculateTangents();
            RecalculateBounds();
        }
    }
}
