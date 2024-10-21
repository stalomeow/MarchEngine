using March.Core.IconFonts;
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

            foreach ((AssetCategory category, string path) in AssetDatabase.GetAllAssetCategoriesAndPaths())
            {
                AddPath(category, path);
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

        private void AddPath(AssetCategory category, string path)
        {
            if (category == AssetCategory.ProjectAsset)
            {
                m_ProjectTree.Add(path, AssetDatabase.IsFolder(path));
            }
            else if (category == AssetCategory.EngineShader)
            {
                m_EngineFileTree.Add(path, AssetDatabase.IsFolder(path));
            }
        }

        private void RemovePath(AssetCategory category, string path)
        {
            if (category == AssetCategory.ProjectAsset)
            {
                m_ProjectTree.Remove(path, AssetDatabase.IsFolder(path));
            }
            else if (category == AssetCategory.EngineShader)
            {
                m_EngineFileTree.Remove(path, AssetDatabase.IsFolder(path));
            }
        }

        private void OnAssetRenamed(AssetCategory oldCategory, string oldPath, AssetCategory newCategory, string newPath)
        {
            RemovePath(oldCategory, oldPath);
            AddPath(newCategory, newPath);
        }
    }
}
