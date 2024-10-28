using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".dds", ".jpg", ".jpeg", ".png")]
    internal class TextureImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Texture Asset";

        protected override int Version => base.Version + 5;

        public override string IconNormal => FontAwesome6.Image;

        protected override bool UseCache => true;

        [JsonProperty]
        [InspectorName("sRGB")]
        public bool IsSRGB { get; set; } = true;

        [JsonProperty("FilterMode")]
        [InspectorName("Filter Mode")]
        public FilterMode Filter { get; set; } = FilterMode.Bilinear;

        [JsonProperty("WrapMode")]
        [InspectorName("Wrap Mode")]
        public WrapMode Wrap { get; set; } = WrapMode.Repeat;

        protected override MarchObject CreateAsset()
        {
            return new Texture();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            Texture texture = (Texture)asset;
            texture.SourceType = Path.GetExtension(AssetFullPath).Equals(".dds", StringComparison.OrdinalIgnoreCase) ? Texture2DSourceType.DDS : Texture2DSourceType.WIC;
            texture.IsSRGB = IsSRGB;
            texture.Filter = Filter;
            texture.Wrap = Wrap;

            string name = Path.GetFileNameWithoutExtension(AssetPath);
            byte[] source = File.ReadAllBytes(AssetFullPath);

            texture.LoadFromSource(name, source);

            if (willSaveToFile)
            {
                texture.SetSerializationData(name, source);
            }
        }

        protected override void OnDidSaveAsset(MarchObject asset)
        {
            Texture texture = (Texture)asset;
            texture.SetSerializationData(string.Empty, []);

            base.OnDidSaveAsset(asset);
        }
    }
}
