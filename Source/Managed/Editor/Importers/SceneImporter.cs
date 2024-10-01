using March.Core;
using March.Core.Serialization;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".scene")]
    internal class SceneImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Scene Asset";

        protected override int Version => base.Version + 1;

        protected override bool UseCache => false;

        protected override MarchObject CreateAsset()
        {
            return new SceneAsset();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }
    }
}
