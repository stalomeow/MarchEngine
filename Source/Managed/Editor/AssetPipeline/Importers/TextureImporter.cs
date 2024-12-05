using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Texture Asset", ".dds", ".jpg", ".jpeg", ".png", Version = 7)]
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
        public bool Compress { get; set; } = true;

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
                Compress = Compress,
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
                EditorGUI.DrawTexture(texture);

                using var info = StringBuilderPool.Get();
                TextureDesc desc = texture.Desc;

                // Display Format
                info.Value.Append(desc.Format.ToString());
                EditorGUI.CenterText(info);

                // Display Resolution And Size
                info.Value.Clear();
                info.Value.Append(desc.Width).Append('x').Append(desc.Height).Append(' ').AppendSize(texture.PixelsSize);
                EditorGUI.CenterText(info);
            }
        }
    }
}
