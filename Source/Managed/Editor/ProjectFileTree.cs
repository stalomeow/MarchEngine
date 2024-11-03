using March.Core.Pool;
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
                AssetImporter? importer = AssetDatabase.GetAssetImporter(node.FolderPath);

                if (BeginAssetTreeNode(name, importer, importer?.MainAssetGuid, isLeaf: false))
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

                bool isLeaf = guids.Value.Count == 1;
                if (BeginAssetTreeNode(name, importer, importer.MainAssetGuid, isLeaf))
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

                                if (BeginAssetTreeNode(importer.GetAssetName(guid)!, importer, guid, isLeaf: true))
                                {
                                    EditorGUI.EndTreeNode();
                                }
                            }
                        }
                    }

                    EditorGUI.EndTreeNode();
                }
            }

            private static bool BeginAssetTreeNode(string name, AssetImporter? importer, string? assetGuid, bool isLeaf)
            {
                bool selectable = (importer != null) && (assetGuid == importer.MainAssetGuid);
                bool selected = selectable && (Selection.Active == importer);
                bool defaultOpen = (importer == null);

                string icon;
                using var id = EditorGUIUtility.BuildId(name);
                if (EditorGUI.IsTreeNodeOpen(id, defaultOpen))
                {
                    icon = importer?.GetAssetExpandedIcon(assetGuid!) ?? FolderImporter.FolderIconExpanded;
                }
                else
                {
                    icon = importer?.GetAssetNormalIcon(assetGuid!) ?? FolderImporter.FolderIconNormal;
                }

                using var label = EditorGUIUtility.BuildIconText(icon, name);
                bool isOpen = EditorGUI.BeginTreeNode(label, isLeaf: isLeaf, selected: selected, defaultOpen: defaultOpen,
                    openOnArrow: true, openOnDoubleClick: true, spanWidth: true);

                if (importer != null && assetGuid != null && DragDrop.BeginSource())
                {
                    using var tooltip = EditorGUIUtility.BuildAssetPath(importer, assetGuid);
                    DragDrop.EndSource(tooltip, importer.GetAsset(assetGuid)!);
                }

                if (selectable && EditorGUI.IsTreeNodeClicked(isOpen, isLeaf) == EditorGUI.ItemClickResult.True)
                {
                    Selection.Active = importer;
                }

                return isOpen;
            }

            public void Clear()
            {
                Folders.Clear();
                Files.Clear();
            }
        }

        private readonly FolderNode m_Root = new("Root");

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
        }
    }
}
