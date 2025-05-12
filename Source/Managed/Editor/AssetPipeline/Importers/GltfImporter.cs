using glTFLoader;
using glTFLoader.Schema;
using March.Core;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Numerics;
using Material = March.Core.Rendering.Material;
using Mesh = March.Core.Rendering.Mesh;
using Scene = March.Core.Scene;
using Texture = March.Core.Rendering.Texture;

namespace March.Editor.AssetPipeline.Importers
{
    public enum ModelNormalImportMode
    {
        Auto,
        Import,
        Calculate,
    }

    public enum ModelTangentImportMode
    {
        Auto,
        Import,
        Calculate,
    }

    // TODO 按照 spec 实现更完整的 glTF 支持

    [CustomAssetImporter("glTF Model Asset", ".gltf", Version = 55)]
    public class GltfImporter : AssetImporter, ISceneDropHandler
    {
        [JsonProperty]
        [InspectorName("Normals")]
        public ModelNormalImportMode NormalImportMode { get; set; } = ModelNormalImportMode.Auto;

        [JsonProperty]
        [InspectorName("Tangents")]
        public ModelTangentImportMode TangentImportMode { get; set; } = ModelTangentImportMode.Auto;

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            using var reader = new GltfReader(ref context, in Location);
            var scene = reader.GetFirstScene();

            GameObject root = context.AddMainAsset("Root", () => new GameObject(), FontAwesome6.DiceD6);
            root.Name = scene.Name ?? Path.GetFileNameWithoutExtension(Location.AssetFullPath);

            using var materials = ListPool<Material>.Get();
            CreateMaterials(ref context, reader, materials);

            using var meshes = ListPool<Mesh>.Get();
            CreateMeshes(ref context, reader, meshes);

            // Reset children
            for (int i = 0; i < root.transform.ChildCount; i++)
            {
                root.transform.GetChild(i).Detach();
            }

            CreateChildren(reader, root, scene.Nodes, materials.Value, meshes.Value);
        }

        private void CreateMaterials(ref AssetImportContext context, GltfReader reader, List<Material> materials)
        {
            if (!reader.Data.ShouldSerializeMaterials())
            {
                return;
            }

            foreach (var m in reader.Data.Materials)
            {
                var asset = context.AddSubAsset<Material>($"Material{materials.Count}");
                asset.Reset();
                asset.Shader = context.RequireOtherAsset<Shader>("Engine/Shaders/Lit.shader", dependsOn: false);
                materials.Add(asset);

                if (m.AlphaMode == glTFLoader.Schema.Material.AlphaModeEnum.MASK)
                {
                    asset.EnableKeyword("_ALPHATEST_ON");
                    asset.SetFloat("_Cutoff", m.AlphaCutoff);
                }

                if (m.DoubleSided)
                {
                    asset.SetInt("_CullMode", (int)CullMode.Off);
                }

                if (m.ShouldSerializePbrMetallicRoughness())
                {
                    if (m.PbrMetallicRoughness.ShouldSerializeBaseColorTexture())
                    {
                        Texture? tex = RequireTexture(ref context, reader, m.PbrMetallicRoughness.BaseColorTexture.Index, true);

                        if (tex != null)
                        {
                            asset.SetTexture("_BaseMap", tex);
                        }
                    }

                    if (m.PbrMetallicRoughness.ShouldSerializeBaseColorFactor())
                    {
                        Color baseColorFactor = new()
                        {
                            R = m.PbrMetallicRoughness.BaseColorFactor[0],
                            G = m.PbrMetallicRoughness.BaseColorFactor[1],
                            B = m.PbrMetallicRoughness.BaseColorFactor[2],
                            A = m.PbrMetallicRoughness.BaseColorFactor[3]
                        };

                        asset.SetColor("_BaseColor", baseColorFactor);
                    }

                    if (m.PbrMetallicRoughness.ShouldSerializeMetallicFactor())
                    {
                        asset.SetFloat("_Metallic", m.PbrMetallicRoughness.MetallicFactor);
                    }

                    if (m.PbrMetallicRoughness.ShouldSerializeRoughnessFactor())
                    {
                        asset.SetFloat("_Roughness", m.PbrMetallicRoughness.RoughnessFactor);
                    }

                    if (m.PbrMetallicRoughness.ShouldSerializeMetallicRoughnessTexture())
                    {
                        Texture? tex = RequireTexture(ref context, reader, m.PbrMetallicRoughness.MetallicRoughnessTexture.Index, false);

                        if (tex != null)
                        {
                            asset.SetTexture("_MetallicRoughnessMap", tex);
                        }
                    }
                }

                if (m.ShouldSerializeNormalTexture())
                {
                    Texture? tex = RequireTexture(ref context, reader, m.NormalTexture.Index, false);

                    if (tex != null)
                    {
                        asset.SetTexture("_BumpMap", tex);
                    }

                    if (m.NormalTexture.ShouldSerializeScale())
                    {
                        asset.SetFloat("_BumpScale", m.NormalTexture.Scale);
                    }
                }

                if (m.ShouldSerializeOcclusionTexture())
                {
                    Texture? tex = RequireTexture(ref context, reader, m.OcclusionTexture.Index, false);

                    if (tex != null)
                    {
                        asset.SetTexture("_OcclusionMap", tex);
                    }

                    if (m.OcclusionTexture.ShouldSerializeStrength())
                    {
                        asset.SetFloat("_OcclusionStrength", m.OcclusionTexture.Strength);
                    }
                }

                if (m.ShouldSerializeEmissiveTexture())
                {
                    Texture? tex = RequireTexture(ref context, reader, m.EmissiveTexture.Index, true);

                    if (tex != null)
                    {
                        asset.SetTexture("_EmissionMap", tex);
                    }
                }

                if (m.ShouldSerializeEmissiveFactor())
                {
                    asset.SetColor("_EmissionColor", new Color
                    {
                        R = m.EmissiveFactor[0],
                        G = m.EmissiveFactor[1],
                        B = m.EmissiveFactor[2],
                        A = 1
                    });
                }
            }
        }

        private Texture? RequireTexture(ref AssetImportContext context, GltfReader reader, int textureIndex, bool isSRGB)
        {
            var t = reader.Data.Textures[textureIndex];

            if (t.Source == null)
            {
                return null;
            }

            string uri = AssetLocationUtility.CombinePath(Path.GetDirectoryName(Location.AssetPath)!, reader.Data.Images[t.Source.Value].Uri);
            return context.RequireOtherAsset<Texture>(uri, dependsOn: false, settings: importer =>
            {
                TextureImporter texImporter = (TextureImporter)importer;
                texImporter.IsSRGB = isSRGB;

                if (t.Sampler != null)
                {
                    var sampler = reader.Data.Samplers[t.Sampler.Value];

                    // 暂时不支持 U 和 V 独立设置
                    if (sampler.ShouldSerializeWrapS())
                    {
                        texImporter.Wrap = sampler.WrapS switch
                        {
                            Sampler.WrapSEnum.CLAMP_TO_EDGE => TextureWrapMode.Clamp,
                            Sampler.WrapSEnum.REPEAT => TextureWrapMode.Repeat,
                            Sampler.WrapSEnum.MIRRORED_REPEAT => TextureWrapMode.Mirror,
                            _ => TextureWrapMode.Repeat,
                        };
                    }

                    // 暂时随便设置一下
                    if (sampler.ShouldSerializeMagFilter())
                    {
                        if (sampler.MagFilter == Sampler.MagFilterEnum.NEAREST)
                        {
                            texImporter.Filter = TextureFilterMode.Point;
                        }
                        else if (sampler.ShouldSerializeMinFilter())
                        {
                            if (sampler.MinFilter == Sampler.MinFilterEnum.LINEAR_MIPMAP_NEAREST)
                            {
                                texImporter.Filter = TextureFilterMode.Bilinear;
                            }
                            else if (sampler.MinFilter == Sampler.MinFilterEnum.LINEAR)
                            {
                                texImporter.Filter = TextureFilterMode.Trilinear;
                            }
                            else if (sampler.MinFilter == Sampler.MinFilterEnum.LINEAR_MIPMAP_LINEAR)
                            {
                                texImporter.Filter = TextureFilterMode.Trilinear;
                            }
                        }
                    }
                }
            });
        }

        private void CreateMeshes(ref AssetImportContext context, GltfReader reader, List<Mesh> meshes)
        {
            if (!reader.Data.ShouldSerializeMeshes())
            {
                return;
            }

            foreach (var m in reader.Data.Meshes)
            {
                var asset = context.AddSubAsset<Mesh>($"Mesh{meshes.Count}");
                asset.ClearSubMeshes();
                meshes.Add(asset);

                int normalCounter = 0;
                int tangentCounter = 0;

                foreach (var primitive in m.Primitives)
                {
                    using var positions = ListPool<Vector3>.Get();
                    using var normals = ListPool<Vector3>.Get();
                    using var tangents = ListPool<Vector4>.Get();
                    using var uvs = ListPool<Vector2>.Get();

                    foreach (KeyValuePair<string, int> kv in primitive.Attributes)
                    {
                        Accessor accessor = reader.Data.Accessors[kv.Value];

                        switch (kv.Key)
                        {
                            case "POSITION":
                                reader.ReadVector3List(accessor, positions, GltfReader.FixPosition);
                                break;

                            case "NORMAL":
                                reader.ReadVector3List(accessor, normals, GltfReader.FixNormal);
                                normalCounter++;
                                break;

                            case "TANGENT":
                                reader.ReadVector4List(accessor, tangents, GltfReader.FixTangent);
                                tangentCounter++;
                                break;

                            case "TEXCOORD_0":
                                reader.ReadVector2List(accessor, uvs, GltfReader.FixUV);
                                break;
                        }
                    }

                    using var vertices = ListPool<MeshVertex>.Get();

                    for (int i = 0; i < positions.Value.Count; i++)
                    {
                        vertices.Value.Add(new MeshVertex
                        {
                            Position = positions.Value.ElementAtOrDefault(i),
                            Normal = normals.Value.ElementAtOrDefault(i),
                            Tangent = tangents.Value.ElementAtOrDefault(i),
                            UV = uvs.Value.ElementAtOrDefault(i)
                        });
                    }

                    using var indices = ListPool<ushort>.Get();

                    if (primitive.Indices != null)
                    {
                        reader.ReadUInt16List(reader.Data.Accessors[primitive.Indices.Value], indices);
                    }
                    else
                    {
                        for (int i = 0; i < positions.Value.Count; i++)
                        {
                            indices.Value.Add((ushort)i);
                        }
                    }

                    GltfReader.FixIndices(indices);

                    asset.AddSubMesh(vertices.Value, indices.Value);
                }

                if ((normalCounter < m.Primitives.Length && NormalImportMode == ModelNormalImportMode.Auto) ||
                    NormalImportMode == ModelNormalImportMode.Calculate)
                {
                    asset.RecalculateNormals();
                }

                if ((tangentCounter < m.Primitives.Length && TangentImportMode == ModelTangentImportMode.Auto) ||
                    TangentImportMode == ModelTangentImportMode.Calculate)
                {
                    asset.RecalculateTangents();
                }

                asset.RecalculateBounds();
            }
        }

        private static void CreateChildren(GltfReader reader, GameObject parent, int[]? nodeIndices, IReadOnlyList<Material> materials, IReadOnlyList<Mesh> meshes)
        {
            if (nodeIndices == null || nodeIndices.Length == 0)
            {
                return;
            }

            foreach (int nodeIdx in nodeIndices)
            {
                var node = reader.Data.Nodes[nodeIdx];
                var go = new GameObject();

                if (!string.IsNullOrWhiteSpace(node.Name))
                {
                    go.Name = node.Name;
                }

                GltfReader.ReadNodeTransform(node, out Vector3 position, out Quaternion rotation, out Vector3 scale);
                go.transform.LocalPosition = position;
                go.transform.LocalRotation = rotation;
                go.transform.LocalScale = scale;

                if (node.Mesh != null)
                {
                    MeshRenderer renderer = go.AddComponent<MeshRenderer>();
                    renderer.Mesh = meshes[node.Mesh.Value];

                    renderer.Materials.Clear();
                    foreach (var primitive in reader.Data.Meshes[node.Mesh.Value].Primitives)
                    {
                        if (primitive.Material != null)
                        {
                            renderer.Materials.Add(materials[primitive.Material.Value]);
                        }
                        else
                        {
                            renderer.Materials.Add(null);
                        }
                    }
                    renderer.SyncNativeMaterials();
                }

                parent.transform.AddChild(go.transform);
                CreateChildren(reader, go, node.Children, materials, meshes);
            }
        }

        void ISceneDropHandler.Execute(Scene scene)
        {
            GameObject prefab = (GameObject)MainAsset;
            scene.AddRootGameObject(Instantiate(prefab));
        }
    }

    internal sealed class GltfReader : IDisposable
    {
        public Gltf Data { get; }

        private readonly BinaryReader[] m_BufferReaders;

        public GltfReader(ref AssetImportContext context, in AssetLocation location)
        {
            using (FileStream fs = File.OpenRead(location.AssetFullPath))
            {
                Data = Interface.LoadModel(fs);
            }

            m_BufferReaders = new BinaryReader[Data.Buffers.Length];
            string directory = Path.GetDirectoryName(location.AssetPath)!;

            for (int i = 0; i < m_BufferReaders.Length; i++)
            {
                string path = AssetLocationUtility.CombinePath(directory, Data.Buffers[i].Uri);
                BinaryAsset bin = context.RequireOtherAsset<BinaryAsset>(path, dependsOn: true);
                m_BufferReaders[i] = new BinaryReader(new MemoryStream(bin.Data));
            }
        }

        public void Dispose()
        {
            foreach (BinaryReader reader in m_BufferReaders)
            {
                reader.Dispose();
            }
        }

        public glTFLoader.Schema.Scene GetFirstScene()
        {
            return Data.Scenes[Data.Scene ?? 0];
        }

        public void ReadUInt16List(Accessor accessor, List<ushort> list)
        {
            ReadBuffer(accessor, reader => list.Add(ReadAsUInt16(reader, accessor.ComponentType)));
        }

        public void ReadVector2List(Accessor accessor, List<Vector2> list, Func<Vector2, Vector2>? fixFunc = null)
        {
            ReadBuffer(accessor, reader =>
            {
                var item = new Vector2
                {
                    X = ReadAsFloat(reader, accessor.ComponentType),
                    Y = ReadAsFloat(reader, accessor.ComponentType)
                };

                if (fixFunc != null)
                {
                    item = fixFunc(item);
                }

                list.Add(item);
            });
        }

        public void ReadVector3List(Accessor accessor, List<Vector3> list, Func<Vector3, Vector3>? fixFunc = null)
        {
            ReadBuffer(accessor, reader =>
            {
                var item = new Vector3
                {
                    X = ReadAsFloat(reader, accessor.ComponentType),
                    Y = ReadAsFloat(reader, accessor.ComponentType),
                    Z = ReadAsFloat(reader, accessor.ComponentType)
                };

                if (fixFunc != null)
                {
                    item = fixFunc(item);
                }

                list.Add(item);
            });
        }

        public void ReadVector4List(Accessor accessor, List<Vector4> list, Func<Vector4, Vector4>? fixFunc = null)
        {
            ReadBuffer(accessor, reader =>
            {
                var item = new Vector4
                {
                    X = ReadAsFloat(reader, accessor.ComponentType),
                    Y = ReadAsFloat(reader, accessor.ComponentType),
                    Z = ReadAsFloat(reader, accessor.ComponentType),
                    W = ReadAsFloat(reader, accessor.ComponentType)
                };

                if (fixFunc != null)
                {
                    item = fixFunc(item);
                }

                list.Add(item);
            });
        }

        private void ReadBuffer(Accessor accessor, Action<BinaryReader> readAction)
        {
            var bufferView = Data.BufferViews[accessor.BufferView!.Value];
            var bufferReader = m_BufferReaders[bufferView.Buffer];

            int offset = accessor.ByteOffset + bufferView.ByteOffset;

            if (bufferView.ByteStride != null)
            {
                int stride = bufferView.ByteStride.Value;

                for (int i = 0; i < accessor.Count; i++)
                {
                    bufferReader.BaseStream.Seek(offset + stride * i, SeekOrigin.Begin);
                    readAction(bufferReader);
                }
            }
            else
            {
                bufferReader.BaseStream.Seek(offset, SeekOrigin.Begin);

                for (int i = 0; i < accessor.Count; i++)
                {
                    readAction(bufferReader);
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

        public static void ReadNodeTransform(Node node, out Vector3 position, out Quaternion rotation, out Vector3 scale)
        {
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

                matrix = Matrix4x4.Transpose(matrix); // glTF 是列主序，但我们是行主序
                Matrix4x4.Decompose(matrix, out scale, out rotation, out position);
            }
            else
            {
                position = Vector3.Zero;
                rotation = Quaternion.Identity;
                scale = Vector3.One;

                if (node.ShouldSerializeTranslation())
                {
                    for (int i = 0; i < 3; i++)
                    {
                        position[i] = node.Translation[i];
                    }
                }

                if (node.ShouldSerializeRotation())
                {
                    for (int i = 0; i < 4; i++)
                    {
                        rotation[i] = node.Rotation[i];
                    }
                }

                if (node.ShouldSerializeScale())
                {
                    for (int i = 0; i < 3; i++)
                    {
                        scale[i] = node.Scale[i];
                    }
                }
            }

            position = FixPosition(position);
            rotation = FixRotation(rotation);
            scale = FixScale(scale);
        }

        public static Vector3 FixPosition(Vector3 position)
        {
            // glTF 是右手坐标系
            return new Vector3(-position.X, position.Y, position.Z);
        }

        public static Quaternion FixRotation(Quaternion rotation)
        {
            // glTF 是右手坐标系
            rotation.ToAngleAxis(out float angle, out Vector3 axis);
            axis.X = -axis.X;
            return Quaternion.CreateFromAxisAngle(axis, -angle);
        }

        public static Vector3 FixScale(Vector3 scale)
        {
            return scale;
        }

        public static Vector3 FixNormal(Vector3 normal)
        {
            // glTF 是右手坐标系
            return new Vector3(-normal.X, normal.Y, normal.Z);
        }

        public static Vector4 FixTangent(Vector4 tangent)
        {
            // glTF 是右手坐标系
            return new Vector4(-tangent.X, tangent.Y, tangent.Z, -tangent.W);
        }

        public static Vector2 FixUV(Vector2 uv)
        {
            // glTF 的 UV 坐标系是左上角为原点，不用变
            return uv;
        }

        public static void FixIndices(List<ushort> indices)
        {
            // glTF 逆时针为正面，我们顺时针为正面
            for (int i = 0; i < indices.Count / 3; i++)
            {
                int a = i * 3 + 0;
                int b = i * 3 + 1;
                (indices[a], indices[b]) = (indices[b], indices[a]);
            }
        }
    }
}
