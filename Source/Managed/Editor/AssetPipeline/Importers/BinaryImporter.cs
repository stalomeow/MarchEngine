using March.Core;
using March.Core.IconFont;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Binary Asset", ".bin", Version = 1)]
    public class BinaryImporter : AssetImporter
    {
        protected override void OnImportAssets(ref AssetImportContext context)
        {
            var asset = context.AddMainAsset<BinaryAsset>(normalIcon: FontAwesome6.Database);
            asset.Data = File.ReadAllBytes(Location.AssetFullPath);
        }
    }
}
