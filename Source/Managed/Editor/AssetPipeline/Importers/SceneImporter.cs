using March.Core;
using March.Core.IconFonts;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Scene Asset", ".scene", Version = 1)]
    internal class SceneImporter : DirectAssetImporter
    {
        public const string SceneIcon = FontAwesome6.CubesStacked;

        protected override MarchObject CreateAsset()
        {
            return new Scene();
        }

        protected override string? GetNormalIcon()
        {
            return SceneIcon;
        }
    }
}
