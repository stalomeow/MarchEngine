using glTFLoader;
using glTFLoader.Schema;
using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using System.Numerics;
using Mesh = March.Core.Rendering.Mesh;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".gltf")]
    internal class GltfImporter : ExternalAssetImporter
    {
        public override string DisplayName => "glTF Model Asset";

        protected override int Version => base.Version + 1;

        public override string IconNormal => FontAwesome6.Cube;

        protected override bool UseCache => true;

        protected override MarchObject CreateAsset()
        {
            return new GameObject();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            Gltf gltf;

            using (FileStream fs = File.OpenRead(AssetFullPath))
            {
                gltf = Interface.LoadModel(fs);
            }

            BinaryReader[] buffers = new BinaryReader[gltf.Buffers.Length];
            string directory = Path.GetDirectoryName(AssetFullPath)!;

            for (int i = 0; i < buffers.Length; i++)
            {
                string path = Path.Combine(directory, gltf.Buffers[i].Uri);
                buffers[i] = new BinaryReader(File.OpenRead(path));
            }

            var scene = gltf.Scenes[gltf.Scene ?? 0]; // 只加载一个 Scene

            GameObject root = (GameObject)asset;
            root.Name = scene.Name ?? Path.GetFileNameWithoutExtension(AssetFullPath);
            CreateChildren(gltf, buffers, root, scene.Nodes);

            for (int i = 0; i < buffers.Length; i++)
            {
                buffers[i].Dispose();
            }
        }

        private static void CreateChildren(Gltf gltf, BinaryReader[] buffers, GameObject parent, int[]? nodeIndices)
        {
            if (nodeIndices == null || nodeIndices.Length == 0)
            {
                return;
            }

            foreach (var node in nodeIndices.Select(i => gltf.Nodes[i]))
            {
                var go = new GameObject();

                if (node.ShouldSerializeName())
                {
                    go.Name = node.Name;
                }

                if (node.ShouldSerializeMatrix())
                {
                    var matrix = new Matrix4x4();

                    for (int i = 0; i < 4; i++)
                    {
                        for (int j = 0; j < 4; j++)
                        {
                            matrix[j, i] = node.Matrix[i * 4 + j];
                        }
                    }

                    // 现在还没设置 parent，所以可以直接赋值给 LocalToWorldMatrix
                    go.transform.LocalToWorldMatrix = matrix;
                }
                else
                {
                    if (node.ShouldSerializeTranslation())
                    {
                        var translation = new Vector3();

                        for (int i = 0; i < 3; i++)
                        {
                            translation[i] = node.Translation[i];
                        }

                        go.transform.LocalPosition = translation;
                    }

                    if (node.ShouldSerializeRotation())
                    {
                        var rotation = new Quaternion();

                        for (int i = 0; i < 4; i++)
                        {
                            rotation[i] = node.Rotation[i];
                        }

                        go.transform.LocalRotation = rotation;
                    }

                    if (node.ShouldSerializeScale())
                    {
                        var scale = new Vector3();

                        for (int i = 0; i < 3; i++)
                        {
                            scale[i] = node.Scale[i];
                        }

                        go.transform.LocalScale = scale;
                    }
                }

                if (node.ShouldSerializeMesh())
                {
                    MeshRenderer renderer = go.AddComponent<MeshRenderer>();
                    renderer.Mesh = CreateMesh(gltf, buffers, gltf.Meshes[node.Mesh!.Value]);
                }

                parent.transform.AddChild(go.transform);
                CreateChildren(gltf, buffers, go, node.Children);
            }
        }

        private static Mesh CreateMesh(Gltf gltf, BinaryReader[] buffers, glTFLoader.Schema.Mesh m)
        {
            var mesh = new Mesh();

            foreach (var primitive in m.Primitives)
            {
                using var positions = ListPool<Vector3>.Get();
                using var normals = ListPool<Vector3>.Get();
                using var tangents = ListPool<Vector3>.Get();
                using var uvs = ListPool<Vector2>.Get();

                foreach (KeyValuePair<string, int> kv in primitive.Attributes)
                {
                    switch (kv.Key)
                    {
                        case "POSITION":
                            ReadVector3List(gltf, buffers, gltf.Accessors[kv.Value], positions);
                            break;

                        case "NORMAL":
                            ReadVector3List(gltf, buffers, gltf.Accessors[kv.Value], normals);
                            break;

                        case "TANGENT":
                            ReadVector3List(gltf, buffers, gltf.Accessors[kv.Value], tangents);
                            break;

                        case "TEXCOORD_0":
                            ReadVector2List(gltf, buffers, gltf.Accessors[kv.Value], uvs);
                            break;
                    }
                }

                using var indices = ListPool<ushort>.Get();
                ReadUInt16List(gltf, buffers, gltf.Accessors[primitive.Indices!.Value], indices);

                using var vertices = ListPool<MeshVertex>.Get();

                for (int i = 0; i < positions.Value.Count; i++)
                {
                    MeshVertex v = new();

                    if (i < positions.Value.Count)
                    {
                        v.Position = positions.Value[i];
                    }

                    if (i < normals.Value.Count)
                    {
                        v.Normal = normals.Value[i];
                    }

                    if (i < tangents.Value.Count)
                    {
                        v.Tangent = tangents.Value[i];
                    }

                    if (i < uvs.Value.Count)
                    {
                        v.UV = uvs.Value[i];
                    }

                    vertices.Value.Add(v);
                }

                mesh.AddSubMesh(vertices.Value, indices.Value);
            }

            return mesh;
        }

        private static void ReadUInt16List(Gltf gltf, BinaryReader[] buffers, Accessor accessor, List<ushort> results)
        {
            var bufferView = gltf.BufferViews[accessor.BufferView!.Value];
            var buffer = buffers[bufferView.Buffer];

            int offset = accessor.ByteOffset + bufferView.ByteOffset;

            if (bufferView.ByteStride != null)
            {
                int stride = bufferView.ByteStride.Value;

                for (int i = 0; i < accessor.Count; i++)
                {
                    buffer.BaseStream.Seek(offset + stride * i, SeekOrigin.Begin);
                    results.Add(ReadAsUInt16(buffer, accessor.ComponentType));
                }
            }
            else
            {
                buffer.BaseStream.Seek(offset, SeekOrigin.Begin);

                for (int i = 0; i < accessor.Count; i++)
                {
                    results.Add(ReadAsUInt16(buffer, accessor.ComponentType));
                }
            }
        }

        private static void ReadVector2List(Gltf gltf, BinaryReader[] buffers, Accessor accessor, List<Vector2> results)
        {
            var bufferView = gltf.BufferViews[accessor.BufferView!.Value];
            var buffer = buffers[bufferView.Buffer];

            int offset = accessor.ByteOffset + bufferView.ByteOffset;

            if (bufferView.ByteStride != null)
            {
                int stride = bufferView.ByteStride.Value;

                for (int i = 0; i < accessor.Count; i++)
                {
                    buffer.BaseStream.Seek(offset + stride * i, SeekOrigin.Begin);

                    results.Add(new Vector2
                    {
                        X = ReadAsFloat(buffer, accessor.ComponentType),
                        Y = ReadAsFloat(buffer, accessor.ComponentType)
                    });
                }
            }
            else
            {
                buffer.BaseStream.Seek(offset, SeekOrigin.Begin);

                for (int i = 0; i < accessor.Count; i++)
                {
                    results.Add(new Vector2
                    {
                        X = ReadAsFloat(buffer, accessor.ComponentType),
                        Y = ReadAsFloat(buffer, accessor.ComponentType)
                    });
                }
            }
        }

        private static void ReadVector3List(Gltf gltf, BinaryReader[] buffers, Accessor accessor, List<Vector3> results)
        {
            var bufferView = gltf.BufferViews[accessor.BufferView!.Value];
            var buffer = buffers[bufferView.Buffer];

            int offset = accessor.ByteOffset + bufferView.ByteOffset;

            if (bufferView.ByteStride != null)
            {
                int stride = bufferView.ByteStride.Value;

                for (int i = 0; i < accessor.Count; i++)
                {
                    buffer.BaseStream.Seek(offset + stride * i, SeekOrigin.Begin);

                    results.Add(new Vector3
                    {
                        X = ReadAsFloat(buffer, accessor.ComponentType),
                        Y = ReadAsFloat(buffer, accessor.ComponentType),
                        Z = ReadAsFloat(buffer, accessor.ComponentType)
                    });
                }
            }
            else
            {
                buffer.BaseStream.Seek(offset, SeekOrigin.Begin);

                for (int i = 0; i < accessor.Count; i++)
                {
                    results.Add(new Vector3
                    {
                        X = ReadAsFloat(buffer, accessor.ComponentType),
                        Y = ReadAsFloat(buffer, accessor.ComponentType),
                        Z = ReadAsFloat(buffer, accessor.ComponentType)
                    });
                }
            }
        }

        private static ushort ReadAsUInt16(BinaryReader reader, Accessor.ComponentTypeEnum type) => type switch
        {
            Accessor.ComponentTypeEnum.BYTE => (ushort)reader.ReadSByte(),
            Accessor.ComponentTypeEnum.UNSIGNED_BYTE => reader.ReadByte(),
            Accessor.ComponentTypeEnum.SHORT => (ushort)reader.ReadInt16(),
            Accessor.ComponentTypeEnum.UNSIGNED_SHORT => reader.ReadUInt16(),
            Accessor.ComponentTypeEnum.UNSIGNED_INT => (ushort)reader.ReadUInt32(),
            Accessor.ComponentTypeEnum.FLOAT => (ushort)reader.ReadSingle(),
            _ => throw new NotSupportedException(),
        };

        private static float ReadAsFloat(BinaryReader reader, Accessor.ComponentTypeEnum type) => type switch
        {
            Accessor.ComponentTypeEnum.BYTE => reader.ReadSByte(),
            Accessor.ComponentTypeEnum.UNSIGNED_BYTE => reader.ReadByte(),
            Accessor.ComponentTypeEnum.SHORT => reader.ReadInt16(),
            Accessor.ComponentTypeEnum.UNSIGNED_SHORT => reader.ReadUInt16(),
            Accessor.ComponentTypeEnum.UNSIGNED_INT => reader.ReadUInt32(),
            Accessor.ComponentTypeEnum.FLOAT => reader.ReadSingle(),
            _ => throw new NotSupportedException(),
        };
    }
}
