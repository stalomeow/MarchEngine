using March.Core;
using March.Core.IconFonts;
using March.Editor.AssetPipeline;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Project")]
    internal class ProjectWindow : EditorWindow
    {
        private readonly ProjectFileTree m_ProjectTree = new();
        private readonly ProjectFileTree m_EngineFileTree = new();

        public ProjectWindow() : base($"{FontAwesome6.Folder} Project")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnOpen()
        {
            base.OnOpen();

            using var locations = ListPool<AssetLocation>.Get();
            AssetDatabase.GetAllAssetLocations(locations);

            foreach (AssetLocation location in locations.Value)
            {
                AddPath(location);
            }

            AssetDatabase.OnImported += AddPath;
            AssetDatabase.OnRemoved += RemovePath;
            AssetDatabase.OnRenamed += OnAssetRenamed;
        }

        protected override void OnClose()
        {
            AssetDatabase.OnImported -= AddPath;
            AssetDatabase.OnRemoved -= RemovePath;
            AssetDatabase.OnRenamed -= OnAssetRenamed;

            base.OnClose();
        }

        protected override void OnDraw()
        {
            base.OnDraw();
            m_ProjectTree.Draw();
            m_EngineFileTree.Draw();
        }

        private void AddPath(AssetLocation location)
        {
            if (location.Category == AssetCategory.ProjectAsset)
            {
                m_ProjectTree.Add(location.AssetPath, AssetDatabase.IsFolder(location.AssetPath));
            }
            else if (location.Category == AssetCategory.EngineShader)
            {
                m_EngineFileTree.Add(location.AssetPath, AssetDatabase.IsFolder(location.AssetPath));
            }
        }

        private void RemovePath(AssetLocation location)
        {
            if (location.Category == AssetCategory.ProjectAsset)
            {
                m_ProjectTree.Remove(location.AssetPath, AssetDatabase.IsFolder(location.AssetPath));
            }
            else if (location.Category == AssetCategory.EngineShader)
            {
                m_EngineFileTree.Remove(location.AssetPath, AssetDatabase.IsFolder(location.AssetPath));
            }
        }

        private void OnAssetRenamed(AssetLocation oldLocation, AssetLocation newLocation)
        {
            RemovePath(oldLocation);
            AddPath(newLocation);
        }
    }
}
