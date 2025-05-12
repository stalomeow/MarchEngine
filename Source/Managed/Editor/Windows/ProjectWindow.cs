using March.Core;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Editor.AssetPipeline;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Project")]
    internal class ProjectWindow : EditorWindow
    {
        private readonly ProjectFileTree m_ProjectTree = new();
        private readonly ProjectFileTree m_EngineFileTree = new();

        public ProjectWindow() : base(FontAwesome6.Folder, "Project")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnOpen()
        {
            base.OnOpen();

            using var locations = ListPool<AssetLocation>.Get();
            using var isFolders = ListPool<bool>.Get();
            AssetDatabase.GetAllAssetLocations(locations, isFolders);

            for (int i = 0; i < locations.Value.Count; i++)
            {
                AddPath(locations.Value[i], isFolders.Value[i]);
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
            s_ContextMenu.ShowAsWindowContext();
        }

        private void AddPath(AssetLocation location, bool isFolder)
        {
            if (location.Category == AssetCategory.ProjectAsset)
            {
                m_ProjectTree.Add(location.AssetPath, isFolder);
            }
            else if (location.IsEngineBuiltIn)
            {
                m_EngineFileTree.Add(location.AssetPath, isFolder);
            }
        }

        private void RemovePath(AssetLocation location, bool isFolder)
        {
            if (location.Category == AssetCategory.ProjectAsset)
            {
                m_ProjectTree.Remove(location.AssetPath, isFolder);
            }
            else if (location.IsEngineBuiltIn)
            {
                m_EngineFileTree.Remove(location.AssetPath, isFolder);
            }
        }

        private void OnAssetRenamed(AssetLocation oldLocation, AssetLocation newLocation, bool isFolder)
        {
            RemovePath(oldLocation, isFolder);
            AddPath(newLocation, isFolder);
        }

        private static readonly GenericMenu s_ContextMenu = new("ProjectWindowContextMenu");

        static ProjectWindow()
        {
            s_ContextMenu.AddMenuItem("Create/Shader", (ref object? arg) =>
            {
                string path = EditorApplication.SaveFilePanelInProject("Create Shader", "NewShader", "shader");

                if (!string.IsNullOrEmpty(path))
                {
                    string name = Path.GetFileNameWithoutExtension(path);
                    File.WriteAllText(AssetLocation.FromPath(path).AssetFullPath, $"Shader \"{name}\" {{}}");
                }
            });

            s_ContextMenu.AddMenuItem("Create/Material", (ref object? arg) =>
            {
                string path = EditorApplication.SaveFilePanelInProject("Create Material", "NewMaterial", "mat");

                if (!string.IsNullOrEmpty(path))
                {
                    AssetDatabase.Create(path, new Material());
                }
            });

            s_ContextMenu.AddMenuItem("Reimport", (ref object? arg) =>
            {
                foreach (MarchObject obj in Selection.Objects)
                {
                    if (obj is AssetImporter importer)
                    {
                        importer.ReimportAndSave(AssetReimportMode.Force);
                    }
                }

            }, enabled: (ref object? arg) => Selection.All<AssetImporter>());
        }
    }
}
