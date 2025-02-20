using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Editor.ShaderLab;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Shader Include Asset", ".hlsl", Version = 2)]
    public class ShaderIncludeImporter : AssetImporter
    {
        protected override void OnImportAssets(ref AssetImportContext context)
        {
            var asset = context.AddMainAsset<ShaderIncludeAsset>(normalIcon: FontAwesome6.FileLines);
            asset.Text = File.ReadAllText(Location.AssetFullPath, Encoding.UTF8);

            using var includes = ListPool<AssetLocation>.Get();
            ShaderProgramUtility.GetIncludeFiles(Location.AssetFullPath, asset.Text, includes);

            foreach (AssetLocation location in includes.Value)
            {
                context.RequireOtherAsset<ShaderIncludeAsset>(location.AssetPath, dependsOn: true);
            }
        }
    }
}
