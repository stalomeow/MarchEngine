using March.Core;
using March.Core.Rendering;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".mat")]
    internal class MaterialImporter : DirectAssetImporter
    {
        public override string DisplayName => "Material Asset";

        protected override int Version => base.Version + 1;

        protected override MarchObject CreateAsset()
        {
            return new Material();
        }
    }
}
