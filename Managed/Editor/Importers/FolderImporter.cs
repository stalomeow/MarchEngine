using DX12Demo.Core;

namespace DX12Demo.Editor.Importers
{
    /// <summary>
    /// 特殊的 <see cref="AssetImporter"/>，用于导入文件夹
    /// </summary>
    internal class FolderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Folder Asset";

        protected override int Version => 1;

        protected override bool UseCache => false;

        protected override EngineObject CreateAsset()
        {
            return s_SharedAsset;
        }

        protected override void PopulateAsset(EngineObject asset, bool willSaveToFile) { }

        private static readonly FolderAsset s_SharedAsset = new();
    }
}
