using March.Core;
using March.Editor.IconFont;

namespace March.Editor.AssetPipeline.Importers
{
    /// <summary>
    /// 特殊的 <see cref="AssetImporter"/>，用于导入文件夹
    /// </summary>
    [CustomAssetImporter("Folder Asset", Version = 1)]
    public sealed class FolderImporter : AssetImporter
    {
        public const string FolderIconNormal = FontAwesome6.Folder;
        public const string FolderIconExpanded = FontAwesome6.FolderOpen;

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            context.AddMainAsset<FolderAsset>(normalIcon: FolderIconNormal, expandedIcon: FolderIconExpanded);
        }

        protected override MarchObject LoadAssetFromCache(string guid) => new FolderAsset();

        protected override bool HasValidAssetCache(string guid) => true;

        protected override void SaveAssetToCache(MarchObject asset) { }

        protected override void DeleteAssetCache(string guid) { }
    }
}
