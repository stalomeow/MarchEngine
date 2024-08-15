using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using Newtonsoft.Json;

namespace DX12Demo.Editor.Importers
{
    [CustomAssetImporter(".dds")]
    internal class TextureImporter : AssetImporter
    {
        public override string DisplayName => "Texture Asset";

        protected override int Version => 1;

        protected override bool UseCache => true;

        [JsonProperty("Filter Mode")]
        public FilterMode Filter { get; set; }

        [JsonProperty("Wrap Mode")]
        public WrapMode Wrap { get; set; }

        protected override EngineObject CreateAsset(bool willSaveToFile)
        {
            Texture texture = new()
            {
                Filter = Filter,
                Wrap = Wrap,
            };

            string name = Path.GetFileNameWithoutExtension(AssetPath);
            byte[] ddsData = File.ReadAllBytes(AssetFullPath);

            texture.SetDDSData(name, ddsData);

            if (willSaveToFile)
            {
                texture.SetSerializationData(name, ddsData);
            }

            return texture;
        }

        protected override void OnDidSaveAsset(EngineObject asset)
        {
            Texture texture = (Texture)asset;
            texture.SetSerializationData(string.Empty, []);

            base.OnDidSaveAsset(asset);
        }
    }
}
