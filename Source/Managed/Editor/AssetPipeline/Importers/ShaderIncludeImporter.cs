using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.ShaderLab;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Shader Include Asset", ".hlsl", Version = 4)]
    public class ShaderIncludeImporter : AssetImporter
    {
        protected override void OnImportAssets(ref AssetImportContext context)
        {
            var asset = context.AddMainAsset<ShaderIncludeAsset>(normalIcon: FontAwesome6.FileLines);
            asset.Text = File.ReadAllText(Location.AssetFullPath, Encoding.UTF8);

            using var locations = ListPool<AssetLocation>.Get();
            ShaderCompiler.GetHLSLIncludesAndPragmas(asset.Text, out List<string> includes, out _);
            ShaderProgramUtility.GetHLSLIncludeLocations(Location.AssetFullPath, includes, locations);

            foreach (AssetLocation location in locations.Value)
            {
                context.RequireOtherAsset<ShaderIncludeAsset>(location.AssetPath, dependsOn: true);
            }
        }
    }
}
