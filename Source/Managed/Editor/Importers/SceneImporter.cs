using March.Core;
using March.Core.IconFonts;
using March.Core.Serialization;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".scene")]
    internal class SceneImporter : ExternalAssetImporter
    {
        public const string SceneIcon = FontAwesome6.CubesStacked;

        public override string DisplayName => "Scene Asset";

        protected override int Version => base.Version + 1;

        public override string IconNormal => SceneIcon;

        protected override bool UseCache => false;

        protected override MarchObject CreateAsset()
        {
            return new Scene();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }
    }
}
