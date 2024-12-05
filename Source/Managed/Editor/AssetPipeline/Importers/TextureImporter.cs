using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Texture Asset", ".dds", ".jpg", ".jpeg", ".png", Version = 9)]
    internal class TextureImporter : AssetImporter
    {
        [JsonProperty]
        [InspectorName("sRGB")]
        public bool IsSRGB { get; set; } = true;

        [JsonProperty]
        [InspectorName("Generate Mipmaps")]
        public bool GenerateMipmaps { get; set; } = true;

        [JsonProperty]
        [InspectorName("Unordered Access")]
        public bool UnorderedAccess { get; set; } = false;

        [JsonProperty]
        public TextureCompression Compression { get; set; } = TextureCompression.NormalQuality;

        [JsonProperty]
        [InspectorName("Filter Mode")]
        public TextureFilterMode Filter { get; set; } = TextureFilterMode.Bilinear;

        [JsonProperty]
        [InspectorName("Wrap Mode")]
        public TextureWrapMode Wrap { get; set; } = TextureWrapMode.Repeat;

        [JsonProperty]
        [InspectorName("Mipmap Bias")]
        public float MipmapBias { get; set; } = 0.0f;

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            ExternalTexture texture = context.AddMainAsset<ExternalTexture>(normalIcon: FontAwesome6.Image);

            LoadTextureFileArgs args = new()
            {
                Filter = Filter,
                Wrap = Wrap,
                MipmapBias = MipmapBias,
                Compression = Compression,
            };

            if (IsSRGB)
            {
                args.Flags |= TextureFlags.SRGB;
            }

            if (GenerateMipmaps)
            {
                args.Flags |= TextureFlags.Mipmaps;
            }

            if (UnorderedAccess)
            {
                args.Flags |= TextureFlags.UnorderedAccess;
            }

            string name = Path.GetFileNameWithoutExtension(Location.AssetFullPath);
            texture.LoadFromFile(name, Location.AssetFullPath, in args);
        }
    }

    internal class TextureImporterDrawer : AssetImporterDrawerFor<TextureImporter>
    {
        protected override void DrawAdditional()
        {
            base.DrawAdditional();

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Preview", string.Empty, defaultOpen: true))
            {
                var texture = (ExternalTexture)Target.MainAsset;
                TextureDesc desc = texture.Desc;

                if (desc.Dimension == TextureDimension.Tex2D)
                {
                    EditorGUI.DrawTexture(texture);
                }

                using var info = StringBuilderPool.Get();

                // Display Format And Dimension
                info.Value.Append(desc.Format.ToString());
                info.Value.Append(",  ");
                AppendTextureDimension(info.Value, desc.Dimension);
                EditorGUI.CenterText(info);

                // Display Resolution And Size
                info.Value.Clear();
                info.Value.Append(desc.Width).Append('x').Append(desc.Height);

                if (desc.Dimension is (TextureDimension.Tex3D or TextureDimension.Tex2DArray or TextureDimension.CubeArray))
                {
                    info.Value.Append('x').Append(desc.DepthOrArraySize);
                }

                info.Value.Append(",  ");
                info.Value.AppendSize(texture.PixelsSize);
                EditorGUI.CenterText(info);
            }
        }

        private static void AppendTextureDimension(StringBuilder builder, TextureDimension dimension)
        {
            switch (dimension)
            {
                case TextureDimension.Tex2D:
                    builder.Append("2D");
                    break;
                case TextureDimension.Tex3D:
                    builder.Append("3D");
                    break;
                case TextureDimension.Cube:
                    builder.Append("Cubemap");
                    break;
                case TextureDimension.Tex2DArray:
                    builder.Append("2D Array");
                    break;
                case TextureDimension.CubeArray:
                    builder.Append("Cubemap Array");
                    break;
                default:
                    builder.Append("Unknown");
                    break;
            }
        }
    }
}
