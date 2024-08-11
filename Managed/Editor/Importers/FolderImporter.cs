using DX12Demo.Core;

namespace DX12Demo.Editor.Importers
{
    /// <summary>
    /// 特殊的 <see cref="AssetImporter"/>，用于导入文件夹
    /// </summary>
    internal class FolderImporter : AssetImporter
    {
        public override string DisplayName => "Folder Asset";

        public override EngineObject Import(string path)
        {
            return new FolderAsset();
        }
    }
}
