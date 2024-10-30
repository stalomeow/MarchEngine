using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Editor.AssetPipeline;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter(".mat")]
    internal class MaterialImporter : DirectAssetImporter
    {
        public override string DisplayName => "Material Asset";

        protected override int Version => base.Version + 1;

        public override string IconNormal => FontAwesome6.Droplet;

        protected override MarchObject CreateAsset()
        {
            return new Material();
        }
    }
}
