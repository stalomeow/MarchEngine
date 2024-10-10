namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Project")]
    internal class ProjectWindow : EditorWindow
    {
        private readonly ProjectFileTree m_FileTree = new();

        public ProjectWindow() : base("Project") { }

        protected override void OnOpen()
        {
            base.OnOpen();

            foreach (string path in AssetDatabase.GetAllProjectAssetPaths())
            {
                AddPath(path);
            }

            AssetDatabase.OnProjectAssetImported += AddPath;
            AssetDatabase.OnProjectAssetRemoved += RemovePath;
            AssetDatabase.OnProjectAssetRenamed += OnAssetRenamed;
        }

        protected override void OnClose()
        {
            AssetDatabase.OnProjectAssetImported -= AddPath;
            AssetDatabase.OnProjectAssetRemoved -= RemovePath;
            AssetDatabase.OnProjectAssetRenamed -= OnAssetRenamed;

            base.OnClose();
        }

        protected override void OnDraw()
        {
            base.OnDraw();
            m_FileTree.Draw();
        }

        private void AddPath(string path)
        {
            m_FileTree.Add(path, AssetDatabase.IsFolder(path));
        }

        private void RemovePath(string path)
        {
            m_FileTree.Remove(path, AssetDatabase.IsFolder(path));
        }

        private void OnAssetRenamed(string oldPath, string newPath)
        {
            RemovePath(oldPath);
            AddPath(newPath);
        }
    }
}
