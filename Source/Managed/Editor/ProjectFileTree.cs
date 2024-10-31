using March.Core;
using March.Editor.AssetPipeline;
using March.Editor.AssetPipeline.Importers;
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
                    DrawFolderAssetNode(name, node);
                }

                foreach ((string name, string path) in Files)
                {
                    DrawLeafAssetNode(name, path);
                }
            }

            private static void DrawFolderAssetNode(string name, FolderNode node)
            {
                // 如果 importer 是 null，说明是 Assets 根结点
                AssetImporter? importer = AssetDatabase.GetAssetImporter(node.FolderPath);

                string icon;
                using var id = EditorGUIUtility.BuildId(name);
                if (EditorGUI.IsTreeNodeOpen(id))
                {
                    icon = importer?.MainAssetExpandedIcon ?? FolderImporter.FolderIconExpanded;
                }
                else
                {
                    icon = importer?.MainAssetNormalIcon ?? FolderImporter.FolderIconNormal;
                }

                using var label = EditorGUIUtility.BuildIconText(icon, name);
                string assetPath = importer?.Location.AssetPath ?? string.Empty;
                string assetGuid = importer?.MainAssetGuid ?? string.Empty;
                bool selected = (importer != null) && (Selection.Active == importer);
                bool defaultOpen = (importer == null); // 默认展开 Assets 根结点
                bool open = EditorGUI.BeginAssetTreeNode(label, assetPath, assetGuid, openOnArrow: true, openOnDoubleClick: true, selected: selected, defaultOpen: defaultOpen, spanWidth: true);

                if (importer != null && EditorGUI.IsTreeNodeClicked(open, isLeaf: false) == EditorGUI.ItemClickResult.True)
                {
                    Selection.Active = importer;
                }

                if (open)
                {
                    node.Draw();
                    EditorGUI.EndTreeNode();
                }
            }

            private static void DrawLeafAssetNode(string name, string path)
            {
                AssetImporter importer = AssetDatabase.GetAssetImporter(path)!;
                using var guids = ListPool<string>.Get();
                importer.GetAssetGuids(guids);

                using var id = EditorGUIUtility.BuildId(name);
                string icon = EditorGUI.IsTreeNodeOpen(id) ? importer.MainAssetExpandedIcon : importer.MainAssetNormalIcon;

                using var label = EditorGUIUtility.BuildIconText(icon, name);
                bool selected = Selection.Active == importer;
                bool isLeaf = guids.Value.Count == 1;
                bool open = EditorGUI.BeginAssetTreeNode(label,
                    importer.Location.AssetPath, importer.MainAssetGuid,
                    openOnArrow: true, openOnDoubleClick: true, isLeaf: isLeaf, selected: selected, spanWidth: true);

                if (EditorGUI.IsTreeNodeClicked(open, isLeaf: isLeaf) == EditorGUI.ItemClickResult.True)
                {
                    Selection.Active = importer;
                }

                if (open)
                {
                    if (!isLeaf)
                    {
                        using (new EditorGUI.IDScope(name))
                        {
                            foreach (string guid in guids.Value)
                            {
                                if (guid == importer.MainAssetGuid)
                                {
                                    continue;
                                }

                                string subAssetIcon = importer.GetAssetNormalIcon(guid)!;
                                string subAssetName = importer.GetAssetName(guid)!;
                                using var subAssetLabel = EditorGUIUtility.BuildIconText(subAssetIcon, subAssetName);

                                if (EditorGUI.BeginAssetTreeNode(subAssetLabel,
                                    importer.Location.AssetPath, guid, isLeaf: true, spanWidth: true))
                                {
                                    EditorGUI.EndTreeNode();
                                }
                            }
                        }
                    }

                    EditorGUI.EndTreeNode();
                }
            }

            public void Clear()
            {
                Folders.Clear();
                Files.Clear();
            }
        }

        private readonly FolderNode m_Root = new("Root");

        private static readonly GenericMenu s_ContextMenu = new("ProjectFileTreeContextMenu");

        static ProjectFileTree()
        {
            s_ContextMenu.AddMenuItem("Create/Shader", (ref object? arg) =>
            {
                var importer = Selection.Active as AssetImporter;
                string folder;

                if (AssetDatabase.IsFolder(importer))
                {
                    folder = importer.Location.AssetFullPath;
                }
                else if (importer == null)
                {
                    folder = Path.Combine(Application.DataPath, "Assets");
                }
                else
                {
                    folder = Path.GetDirectoryName(importer.Location.AssetFullPath)!;
                }

                File.WriteAllText(Path.Combine(folder, "NewShader.shader"), string.Empty);
            }, enabled: (ref object? arg) => Selection.Active is (AssetImporter or null));
        }

        public void AddFile(string path) => Add(path, false);

        public void AddFolder(string path) => Add(path, true);

        public void Add(string path, bool isFolder)
        {
            if (AssetLocation.IsImporterFilePath(path))
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

        public void Remove(string path, bool isFolder)
        {
            if (FindParentNode(ref path, false, out string[] segments, out FolderNode? node))
            {
                string key = segments[^1];

                if (isFolder)
                {
                    node.Folders.Remove(key);
                }
                else
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
            s_ContextMenu.ShowAsWindowContext();
        }
    }
}
