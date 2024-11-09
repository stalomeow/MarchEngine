using glTFLoader;
using glTFLoader.Schema;
using March.Core;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using System.Numerics;
using Material = March.Core.Rendering.Material;
using Mesh = March.Core.Rendering.Mesh;
using Texture = March.Core.Rendering.Texture;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("glTF Model Asset", ".gltf", Version = 17)]
    internal class GltfImporter : AssetImporter
    {
        protected override void OnImportAssets(ref AssetImportContext context)
        {
            Gltf gltf;

            using (FileStream fs = File.OpenRead(Location.AssetFullPath))
            {
                gltf = Interface.LoadModel(fs);
            }

            BinaryReader[] buffers = new BinaryReader[gltf.Buffers.Length];
            string directory = Path.GetDirectoryName(Location.AssetPath)!;

            for (int i = 0; i < buffers.Length; i++)
            {
                string path = AssetLocationUtility.CombinePath(directory, gltf.Buffers[i].Uri);
                BinaryAsset bin = context.RequireOtherAsset<BinaryAsset>(path, dependsOn: true);
                buffers[i] = new BinaryReader(new MemoryStream(bin.Data));
            }

            var scene = gltf.Scenes[gltf.Scene ?? 0]; // 只加载一个 Scene

            GameObject root = context.AddMainAsset("Root", () => new GameObject(), FontAwesome6.DiceD6);
            root.Name = scene.Name ?? Path.GetFileNameWithoutExtension(Location.AssetFullPath);

            // Reset children
            for (int i = 0; i < root.transform.ChildCount; i++)
            {
                root.transform.GetChild(i).Detach();
            }

            CreateChildren(ref context, gltf, buffers, root, scene.Nodes);

            for (int i = 0; i < buffers.Length; i++)
            {
                buffers[i].Dispose();
            }
        }

        private void CreateChildren(ref AssetImportContext context, Gltf gltf, BinaryReader[] buffers, GameObject parent, int[]? nodeIndices)
        {
            if (nodeIndices == null || nodeIndices.Length == 0)
            {
                return;
            }

            foreach (int nodeIdx in nodeIndices)
            {
                var node = gltf.Nodes[nodeIdx];
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
                    renderer.Materials.Clear();
                    renderer.Mesh = CreateMesh(ref context, gltf, buffers, gltf.Meshes[node.Mesh!.Value], renderer.Materials, nodeIdx);
                    renderer.SyncNativeMaterials();
                }

                parent.transform.AddChild(go.transform);
                CreateChildren(ref context, gltf, buffers, go, node.Children);
            }
        }

        private Mesh CreateMesh(ref AssetImportContext context, Gltf gltf, BinaryReader[] buffers, glTFLoader.Schema.Mesh m, List<Material?> materials, int nodeIndex)
        {
            var mesh = context.AddSubAsset<Mesh>($"Node{nodeIndex}_Mesh");
            mesh.ClearSubMeshes();

            foreach (var primitive in m.Primitives)
            {
                using var positions = ListPool<Vector3>.Get();
                using var normals = ListPool<Vector3>.Get();
                using var tangents = ListPool<Vector4>.Get();
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
                            ReadVector4List(gltf, buffers, gltf.Accessors[kv.Value], tangents);
                            break;

                        case "TEXCOORD_0":
                            ReadVector2List(gltf, buffers, gltf.Accessors[kv.Value], uvs);
                            break;
                    }
                }

                using var indices = ListPool<ushort>.Get();

                if (primitive.Indices != null)
                {
                    ReadUInt16List(gltf, buffers, gltf.Accessors[primitive.Indices.Value], indices);
                }
                else
                {
                    for (int i = 0; i < positions.Value.Count; i++)
                    {
                        indices.Value.Add((ushort)i);
                    }
                }

                using var vertices = ListPool<MeshVertex>.Get();

                for (int i = 0; i < positions.Value.Count; i++)
                {
                    MeshVertex v = new();

                    if (i < positions.Value.Count)
                    {
                        v.Position = positions.Value[i];
                        v.Position.X = -v.Position.X; // glTF 是左手坐标系
                    }

                    if (i < normals.Value.Count)
                    {
                        v.Normal = normals.Value[i];
                        v.Normal.X = -v.Normal.X; // glTF 是左手坐标系
                    }

                    if (i < tangents.Value.Count)
                    {
                        v.Tangent = tangents.Value[i];
                        v.Tangent.X = -v.Tangent.X; // glTF 是左手坐标系
                        v.Tangent.W = -v.Tangent.W;
                    }

                    if (i < uvs.Value.Count)
                    {
                        v.UV = uvs.Value[i]; // glTF 的 UV 坐标系是左上角为原点，不用变
                    }

                    vertices.Value.Add(v);
                }

                // glTF 逆时针为正面，我们顺时针为正面
                for (int i = 0; i < indices.Value.Count / 3; i++)
                {
                    int a = i * 3 + 0;
                    int b = i * 3 + 1;
                    (indices.Value[a], indices.Value[b]) = (indices.Value[b], indices.Value[a]);
                }

                mesh.AddSubMesh(vertices.Value, indices.Value);

                if (primitive.Material == null)
                {
                    materials.Add(null);
                }
                else
                {
                    var mat = context.AddSubAsset<Material>($"Node{nodeIndex}_SubMesh{mesh.SubMeshCount}_Material");
                    mat.Reset();
                    materials.Add(mat);

                    mat.Shader = context.RequireOtherAsset<Shader>("Engine/Shaders/BlinnPhong.shader", dependsOn: false);

                    var matData = gltf.Materials[primitive.Material.Value];

                    if (matData.PbrMetallicRoughness.BaseColorTexture != null)
                    {
                        int? sourceIndex = gltf.Textures[matData.PbrMetallicRoughness.BaseColorTexture.Index].Source;

                        if (sourceIndex != null)
                        {
                            string uri = AssetLocationUtility.CombinePath(Path.GetDirectoryName(Location.AssetPath)!, gltf.Images[sourceIndex.Value].Uri);
                            var tex = context.RequireOtherAsset<Texture>(uri, dependsOn: false);
                            mat.SetTexture("_DiffuseMap", tex);
                        }
                    }

                    if (matData.NormalTexture != null)
                    {
                        int? sourceIndex = gltf.Textures[matData.NormalTexture.Index].Source;

                        if (sourceIndex != null)
                        {
                            string uri = AssetLocationUtility.CombinePath(Path.GetDirectoryName(Location.AssetPath)!, gltf.Images[sourceIndex.Value].Uri);
                            var tex = context.RequireOtherAsset<Texture>(uri, dependsOn: false);
                            mat.SetTexture("_BumpMap", tex);
                        }
                    }
                }
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

        private static void ReadVector4List(Gltf gltf, BinaryReader[] buffers, Accessor accessor, List<Vector4> results)
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

                    results.Add(new Vector4
                    {
                        X = ReadAsFloat(buffer, accessor.ComponentType),
                        Y = ReadAsFloat(buffer, accessor.ComponentType),
                        Z = ReadAsFloat(buffer, accessor.ComponentType),
                        W = ReadAsFloat(buffer, accessor.ComponentType)
                    });
                }
            }
            else
            {
                buffer.BaseStream.Seek(offset, SeekOrigin.Begin);

                for (int i = 0; i < accessor.Count; i++)
                {
                    results.Add(new Vector4
                    {
                        X = ReadAsFloat(buffer, accessor.ComponentType),
                        Y = ReadAsFloat(buffer, accessor.ComponentType),
                        Z = ReadAsFloat(buffer, accessor.ComponentType),
                        W = ReadAsFloat(buffer, accessor.ComponentType)
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
