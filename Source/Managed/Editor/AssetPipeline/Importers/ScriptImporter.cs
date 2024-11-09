using March.Core;
using March.Core.IconFont;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("C# Script Asset", ".cs", Version = 3)]
    internal class ScriptImporter : AssetImporter
    {
        protected override void OnImportAssets(ref AssetImportContext context)
        {
            var asset = context.AddMainAsset<ScriptAsset>(normalIcon: FontAwesome6.Code);
            asset.Text = File.ReadAllText(Location.AssetFullPath, Encoding.UTF8);
        }
    }
}
