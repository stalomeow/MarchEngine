using March.Core;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor
{
    internal class ProjectFileTree
    {
        private sealed class FolderNode(string folderPath)
        {
            public string FolderPath { get; } = folderPath;

            public SortedDictionary<string, FolderNode> Folders { get; } = new();

            public SortedDictionary<string, string> Files { get; } = new();

            public void Draw()
            {
                // 没必要加 IDScope，因为每个结点名字都不一样

                foreach ((string name, FolderNode node) in Folders)
                {
                    bool selected = (Selection.Active is AssetImporter importer) && (importer.AssetPath == node.FolderPath);
                    string assetPath = AssetDatabase.IsAsset(node.FolderPath) ? node.FolderPath : string.Empty;
                    bool open = EditorGUI.BeginAssetTreeNode(name, assetPath, openOnArrow: true, openOnDoubleClick: true, selected: selected, spanWidth: true);

                    if (EditorGUI.IsTreeNodeClicked(open, isLeaf: false) == EditorGUI.ItemClickResult.True)
                    {
                        Selection.Active = AssetDatabase.GetAssetImporter(node.FolderPath);
                    }

                    if (open)
                    {
                        node.Draw();
                        EditorGUI.EndTreeNode();
                    }
                }

                foreach ((string name, string path) in Files)
                {
                    bool selected = (Selection.Active is AssetImporter importer) && (importer.AssetPath == path);
                    string assetPath = AssetDatabase.IsAsset(path) ? path : string.Empty;
                    bool open = EditorGUI.BeginAssetTreeNode(name, assetPath, isLeaf: true, selected: selected, spanWidth: true);

                    if (EditorGUI.IsTreeNodeClicked(open, isLeaf: true) == EditorGUI.ItemClickResult.True)
                    {
                        Selection.Active = AssetDatabase.GetAssetImporter(path);
                    }

                    if (open)
                    {
                        EditorGUI.EndTreeNode();
                    }
                }
            }

            public void Clear()
            {
                Folders.Clear();
                Files.Clear();
            }
        }

        private readonly FolderNode m_Root = new("Root");

        private static readonly PopupMenu s_ContextMenu = new("ProjectFileTreeContextMenu");

        static ProjectFileTree()
        {
            s_ContextMenu.AddMenuItem("Create/Shader", (ref object? arg) =>
            {
                var importer = Selection.Active as AssetImporter;
                string folder;

                if (AssetDatabase.IsFolder(importer))
                {
                    folder = importer.AssetFullPath;
                }
                else if (importer == null)
                {
                    folder = Path.Combine(Application.DataPath, "Assets");
                }
                else
                {
                    folder = Path.GetDirectoryName(importer.AssetFullPath)!;
                }

                File.WriteAllText(Path.Combine(folder, "NewShader.shader"), string.Empty);
            }, enabled: (ref object? arg) => Selection.Active is (AssetImporter or null));
        }

        public void AddFile(string path) => Add(path, false);

        public void AddFolder(string path) => Add(path, true);

        public void Add(string path, bool isFolder)
        {
            if (AssetDatabase.IsImporterFilePath(path))
            {
                return;
            }

            if (FindParentNode(ref path, true, out string[] segments, out FolderNode? node))
            {
                if (isFolder)
                {
                    node.Folders.Add(segments[^1], new FolderNode(path));
                }
                else
                {
                    node.Files.Add(segments[^1], path);
                }
            }
        }

        public void Remove(string path)
        {
            if (FindParentNode(ref path, false, out string[] segments, out FolderNode? node))
            {
                string key = segments[^1];

                if (!node.Folders.Remove(key))
                {
                    node.Files.Remove(key);
                }
            }
        }

        private bool FindParentNode(ref string path, bool createIfNotExists, out string[] segments, [NotNullWhen(true)] out FolderNode? node)
        {
            path = path.Replace('\\', '/');
            segments = path.Split('/', StringSplitOptions.RemoveEmptyEntries);

            if (segments.Length == 0)
            {
                node = null;
                return false;
            }

            node = m_Root;

            for (int i = 0; i < segments.Length - 1; i++)
            {
                string seg = segments[i];

                if (!node.Folders.TryGetValue(seg, out FolderNode? n))
                {
                    if (!createIfNotExists)
                    {
                        return false;
                    }

                    n = new FolderNode(string.Join('/', segments[..(i + 1)]));
                    node.Folders.Add(seg, n);
                }

                node = n;
            }

            return true;
        }

        public void Clear()
        {
            m_Root.Clear();
        }

        public void Draw()
        {
            m_Root.Draw();
            s_ContextMenu.DoWindowContext();
        }
    }
}
