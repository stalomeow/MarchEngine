using March.Core;
using March.Core.Rendering;
using Newtonsoft.Json;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".dds")]
    internal class TextureImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Texture Asset";

        protected override int Version => base.Version + 1;

        protected override bool UseCache => true;

        [JsonProperty("Filter Mode")]
        public FilterMode Filter { get; set; }

        [JsonProperty("Wrap Mode")]
        public WrapMode Wrap { get; set; }

        protected override EngineObject CreateAsset()
        {
            return new Texture();
        }

        protected override void PopulateAsset(EngineObject asset, bool willSaveToFile)
        {
            Texture texture = (Texture)asset;
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

        protected override void OnDidSaveAsset(EngineObject asset)
        {
            Texture texture = (Texture)asset;
            texture.SetSerializationData(string.Empty, []);

            base.OnDidSaveAsset(asset);
        }
    }
}
