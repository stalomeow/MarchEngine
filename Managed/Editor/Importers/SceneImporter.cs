using DX12Demo.Core;

namespace DX12Demo.Editor.Importers
{
    [CustomAssetImporter(".scene")]
    internal class SceneImporter : AssetImporter
    {
        public override string DisplayName => "Scene Asset";

        public override EngineObject Import(string path)
        {
            string fullPath = Path.Combine(Application.DataPath, path);
            return PersistentManager.Load<Scene>(fullPath);
        }
    }
}
