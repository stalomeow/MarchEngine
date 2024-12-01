using March.Core;
using March.Core.IconFont;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Texture Asset", ".dds", ".jpg", ".jpeg", ".png", Version = 6)]
    internal class TextureImporter : AssetImporter
    {
        [JsonProperty]
        [InspectorName("sRGB")]
        public bool IsSRGB { get; set; } = true;

        [JsonProperty("FilterMode")]
        [InspectorName("Filter Mode")]
        public FilterMode Filter { get; set; } = FilterMode.Bilinear;

        [JsonProperty("WrapMode")]
        [InspectorName("Wrap Mode")]
        public WrapMode Wrap { get; set; } = WrapMode.Repeat;

        private Texture2DSourceType SourceType
        {
            get
            {
                string ext = Path.GetExtension(Location.AssetFullPath);
                return ext.Equals(".dds", StringComparison.OrdinalIgnoreCase) ? Texture2DSourceType.DDS : Texture2DSourceType.WIC;
            }
        }

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            Texture texture = context.AddMainAsset<Texture>(normalIcon: FontAwesome6.Image);

            texture.SourceType = SourceType;
            texture.IsSRGB = IsSRGB;
            texture.Filter = Filter;
            texture.Wrap = Wrap;

            string name = Path.GetFileNameWithoutExtension(Location.AssetFullPath);
            byte[] source = File.ReadAllBytes(Location.AssetFullPath);
            texture.LoadFromSource(name, source);
            texture.SetSerializationData(name, source);
        }

        protected override void SaveAssetToCache(MarchObject asset)
        {
            base.SaveAssetToCache(asset);

            Texture texture = (Texture)asset;
            texture.SetSerializationData(string.Empty, []);
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
                EditorGUI.DrawTexture((Texture)Target.MainAsset);
            }
        }
    }
}
