using March.Core.IconFont;
using March.Core.Rendering;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Shader Include Asset", ".hlsl", Version = 1)]
    internal class ShaderIncludeImporter : AssetImporter
    {
        protected override void OnImportAssets(ref AssetImportContext context)
        {
            var asset = context.AddMainAsset<ShaderIncludeAsset>(normalIcon: FontAwesome6.FileLines);
            asset.Text = File.ReadAllText(Location.AssetFullPath, Encoding.UTF8);
        }
    }
}
