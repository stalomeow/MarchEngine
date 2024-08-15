using DX12Demo.Core;
using DX12Demo.Core.Serialization;

namespace DX12Demo.Editor.Importers
{
    [CustomAssetImporter(".scene")]
    internal class SceneImporter : AssetImporter
    {
        public override string DisplayName => "Scene Asset";

        protected override int Version => 1;

        protected override bool UseCache => false;

        protected override EngineObject CreateAsset(bool willSaveToFile)
        {
            return PersistentManager.Load<SceneAsset>(AssetFullPath);
        }
    }
}
