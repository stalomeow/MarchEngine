using March.Core;
using March.Core.Diagnostics;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Editor.AssetPipeline;
using March.Editor.AssetPipeline.Importers;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Text;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Project")]
    internal class ProjectWindow : EditorWindow
    {
        private readonly ProjectTreeView m_ProjectTree = new(allowMoveFiles: true);
        private readonly ProjectTreeView m_EngineFileTree = new(allowMoveFiles: false); // 不可以移动引擎内置资源的位置

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

            s_ContextMenu.AddMenuItem("Delete", (ref object? arg) =>
            {
                foreach (MarchObject obj in Selection.Objects)
                {
                    if (obj is not AssetImporter importer)
                    {
                        continue;
                    }

                    if (AssetDatabase.IsFolder(importer))
                    {
                        Directory.Delete(importer.Location.AssetFullPath, recursive: true);
                    }
                    else
                    {
                        File.Delete(importer.Location.AssetFullPath);
                    }
                }

                Selection.Clear();

            }, enabled: (ref object? arg) => Selection.All<AssetImporter>());

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

    internal class ProjectTreeView(bool allowMoveFiles) : TreeView
    {
        private interface INode
        {
            string Name { get; }

            string Path { get; }

            INode? Parent { get; }
        }

        private class FileNode(string name, string path, INode? parent) : INode
        {
            public string Name { get; } = name;

            public string Path { get; } = path;

            public INode? Parent { get; } = parent;
        }

        private class FolderNode(string name, string path, INode? parent) : INode
        {
            public string Name { get; } = name;

            public string Path { get; } = path;

            public INode? Parent { get; } = parent;

            public SortedList<string, FolderNode> Folders { get; } = [];

            public SortedList<string, FileNode> Files { get; } = [];

            public int ChildCount => Folders.Count + Files.Count;

            public object this[int index]
            {
                get
                {
                    // 先 Folder 再 File

                    if (index < Folders.Count)
                    {
                        return Folders.GetValueAtIndex(index);
                    }

                    return Files.GetValueAtIndex(index - Folders.Count);
                }
            }

            public void Clear()
            {
                Folders.Clear();
                Files.Clear();
            }
        }

        private readonly FolderNode m_RootNode = new("Root", string.Empty, null);

        public void AddFile(string path) => Add(path, false);

        public void AddFolder(string path) => Add(path, true);

        public void Add(string path, bool isFolder)
        {
            if (AssetLocation.IsImporterFilePath(path))
            {
                return;
            }

            if (FindParentNode(ref path, createIfNotExists: true, out string[] segments, out FolderNode? node))
            {
                string name = segments[^1];

                if (isFolder)
                {
                    if (!node.Folders.ContainsKey(name))
                    {
                        node.Folders.Add(name, new FolderNode(name, path, node));
                    }
                }
                else
                {
                    if (!node.Files.ContainsKey(name))
                    {
                        node.Files.Add(name, new FileNode(name, path, node));
                    }
                }
            }
        }

        public void Remove(string path, bool isFolder)
        {
            if (FindParentNode(ref path, createIfNotExists: false, out string[] segments, out FolderNode? node))
            {
                string name = segments[^1];

                if (isFolder)
                {
                    node.Folders.Remove(name);
                }
                else
                {
                    node.Files.Remove(name);
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

            node = m_RootNode;

            for (int i = 0; i < segments.Length - 1; i++)
            {
                string seg = segments[i];

                if (!node.Folders.TryGetValue(seg, out FolderNode? n))
                {
                    if (!createIfNotExists)
                    {
                        return false;
                    }

                    n = new FolderNode(seg, string.Join('/', segments[..(i + 1)]), node);
                    node.Folders.Add(seg, n);
                }

                node = n;
            }

            return true;
        }

        public void Clear()
        {
            m_RootNode.Clear();
        }

        protected override int GetChildCount(object? item)
        {
            if (item == null)
            {
                return m_RootNode.ChildCount;
            }

            if (item is FolderNode folder)
            {
                return folder.ChildCount;
            }

            // FileNode
            return 0;
        }

        protected override object GetChildItem(object? item, int index)
        {
            if (item == null)
            {
                return m_RootNode[index];
            }

            return ((FolderNode)item)[index];
        }

        protected override object? GetItemByEngineObject(MarchObject obj)
        {
            if (obj is not AssetImporter importer)
            {
                return null;
            }

            string path = importer.Location.AssetPath;

            if (FindParentNode(ref path, createIfNotExists: false, out string[] segments, out FolderNode? folder))
            {
                if (AssetDatabase.IsFolder(importer))
                {
                    if (folder.Folders.TryGetValue(segments[^1], out FolderNode? n))
                    {
                        return n;
                    }
                }
                else
                {
                    if (folder.Files.TryGetValue(segments[^1], out FileNode? n))
                    {
                        return n;
                    }
                }
            }

            return null;
        }

        protected override TreeViewItemDesc GetItemDesc(object item, StringBuilder labelBuilder)
        {
            INode node = (INode)item;
            AssetImporter? importer = AssetDatabase.GetAssetImporter(node.Path);
            bool isOpenByDefault = importer == null;

            string icon;
            using var id = EditorGUIUtility.BuildId(node.Name);

            if (EditorGUI.IsTreeNodeOpen(id, isOpenByDefault))
            {
                icon = importer?.MainAssetExpandedIcon ?? FolderImporter.FolderIconExpanded;
            }
            else
            {
                icon = importer?.MainAssetNormalIcon ?? FolderImporter.FolderIconNormal;
            }

            labelBuilder.AppendIconText(icon, node.Name);

            return new TreeViewItemDesc
            {
                IsDisabled = false,
                EngineObject = importer,
                IsOpenByDefault = isOpenByDefault,
            };
        }

        protected override void GetDragTooltip(IReadOnlyList<object> items, StringBuilder tooltipBuilder)
        {
            for (int i = 0; i < items.Count; i++)
            {
                INode node = (INode)items[i];
                AssetImporter importer = AssetDatabase.GetAssetImporter(node.Path)!;

                tooltipBuilder.AppendAssetPath(importer);
                tooltipBuilder.AppendLine();
            }
        }

        protected override bool CanMoveItem(in TreeViewItemMoveData data)
        {
            // 不允许调整文件的顺序，只能拖到某个文件夹上
            if (!allowMoveFiles || data.Position != TreeViewDropPosition.OnItem || data.TargetItem is not FolderNode)
            {
                return false;
            }

            INode movingNode = (INode)data.MovingItem;
            INode targetNode = (INode)data.TargetItem;

            // 不能移动到原本就在的文件夹里
            if (movingNode.Parent == targetNode)
            {
                return false;
            }

            return true;
        }

        protected override void OnMoveItem(in TreeViewItemMoveData data)
        {
            INode srcNode = (INode)data.MovingItem;
            string? srcFullPath = GetFullPath(srcNode.Path);

            if (srcFullPath == null)
            {
                Log.Message(LogLevel.Error, "Can not determine the full path of srcNode", $"{srcNode.Path}");
                return;
            }

            INode dstFolder = (INode)data.TargetItem;
            string? dstFullPath = GetFullPath(dstFolder.Path);

            if (dstFullPath == null)
            {
                Log.Message(LogLevel.Error, "Can not determine the full path of dstFolder", $"{dstFolder.Path}");
                return;
            }

            // 拷贝到目标文件夹
            dstFullPath = Path.Combine(dstFullPath, srcNode.Name);

            if (srcNode is FolderNode)
            {
                Directory.Move(srcFullPath, dstFullPath);
            }
            else
            {
                File.Move(srcFullPath, dstFullPath);
            }
        }

        protected override bool CanHandleExternalDrop(in TreeViewExternalDropData data)
        {
            // 只能拖到某个文件夹上
            if (!allowMoveFiles || data.Position != TreeViewDropPosition.OnItem || data.TargetItem is not FolderNode dstFolder)
            {
                return false;
            }

            if (data.Paths.Count == 0)
            {
                return false;
            }

            string? dstFolderPath = GetFullPath(dstFolder.Path);

            if (dstFolderPath == null)
            {
                Log.Message(LogLevel.Error, "Can not determine the full path of dstFolder", $"{dstFolder.Path}");
                return false;
            }

            foreach (string path in data.Paths)
            {
                string name = Path.GetFileName(path);
                string dstPath = Path.Combine(dstFolderPath, name);

                if (File.Exists(dstPath) || Directory.Exists(dstPath))
                {
                    return false;
                }
            }

            return true;
        }

        protected override void OnHandleExternalDrop(in TreeViewExternalDropData data)
        {
            INode dstFolder = (INode)data.TargetItem;
            string? dstFolderPath = GetFullPath(dstFolder.Path);

            if (dstFolderPath == null)
            {
                Log.Message(LogLevel.Error, "Can not determine the full path of dstFolder", $"{dstFolder.Path}");
                return;
            }

            foreach (string path in data.Paths)
            {
                string name = Path.GetFileName(path);
                string dstPath = Path.Combine(dstFolderPath, name);

                if (Directory.Exists(path))
                {
                    CopyDirectory(path, dstPath, recursive: true);
                }
                else
                {
                    File.Copy(path, dstPath);
                }
            }
        }

        private static string? GetFullPath(string path)
        {
            if (path.Equals("Assets", StringComparison.InvariantCultureIgnoreCase))
            {
                return AssetLocation.GetBaseFullPath(AssetCategory.ProjectAsset);
            }

            AssetLocation location = AssetLocation.FromPath(path);
            return location.Category == AssetCategory.Unknown ? null : location.AssetFullPath;
        }

        // Ref: https://learn.microsoft.com/en-us/dotnet/standard/io/how-to-copy-directories
        private static void CopyDirectory(string sourceDir, string destinationDir, bool recursive)
        {
            // Get information about the source directory
            var dir = new DirectoryInfo(sourceDir);

            // Check if the source directory exists
            if (!dir.Exists)
            {
                throw new DirectoryNotFoundException($"Source directory not found: {dir.FullName}");
            }

            // Cache directories before we start copying
            DirectoryInfo[] dirs = dir.GetDirectories();

            // Create the destination directory
            Directory.CreateDirectory(destinationDir);

            // Get the files in the source directory and copy to the destination directory
            foreach (FileInfo file in dir.GetFiles())
            {
                string targetFilePath = Path.Combine(destinationDir, file.Name);
                file.CopyTo(targetFilePath);
            }

            // If recursive and copying subdirectories, recursively call this method
            if (recursive)
            {
                foreach (DirectoryInfo subDir in dirs)
                {
                    string newDestinationDir = Path.Combine(destinationDir, subDir.Name);
                    CopyDirectory(subDir.FullName, newDestinationDir, true);
                }
            }
        }
    }
}
