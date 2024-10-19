using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".dds")]
    internal class TextureImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Texture Asset";

        protected override int Version => base.Version + 2;

        public override string IconNormal => FontAwesome6.Image;

        protected override bool UseCache => true;

        [JsonProperty]
        [InspectorName("sRGB")]
        public bool IsSRGB { get; set; } = true;

        [JsonProperty]
        [InspectorName("Filter Mode")]
        public FilterMode Filter { get; set; }

        [JsonProperty]
        [InspectorName("Wrap Mode")]
        public WrapMode Wrap { get; set; }

        protected override MarchObject CreateAsset()
        {
            return new Texture();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            Texture texture = (Texture)asset;
            texture.IsSRGB = IsSRGB;
            texture.Filter = Filter;
            texture.Wrap = Wrap;

            string name = Path.GetFileNameWithoutExtension(AssetPath);
            byte[] ddsData = File.ReadAllBytes(AssetFullPath);

            texture.SetDDSData(name, ddsData);

            if (willSaveToFile)
            {
                texture.SetSerializationData(name, ddsData);
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
