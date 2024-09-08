using DX12Demo.Core;
using DX12Demo.Core.Serialization;

namespace DX12Demo.Editor.Importers
{
    [CustomAssetImporter(".scene")]
    internal class SceneImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Scene Asset";

        protected override int Version => base.Version + 1;

        protected override bool UseCache => false;

        protected override EngineObject CreateAsset()
        {
            return new SceneAsset();
        }

        protected override void PopulateAsset(EngineObject asset, bool willSaveToFile)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }
    }
}
