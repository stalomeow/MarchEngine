using March.Core;

namespace March.Editor.Importers
{
    /// <summary>
    /// 特殊的 <see cref="AssetImporter"/>，用于导入文件夹
    /// </summary>
    internal class FolderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Folder Asset";

        protected override int Version => base.Version + 1;

        protected override bool UseCache => false;

        protected override MarchObject CreateAsset()
        {
            return new FolderAsset();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile) { }
    }
}
