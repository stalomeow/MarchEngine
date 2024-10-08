using March.Core;
using March.Core.Rendering;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;

namespace March.Editor.Windows
{
    internal static class ProjectWindow
    {
        private static readonly ProjectFileTree s_FileTree = new();
        private static readonly FileSystemWatcher s_AssetFileWatcher;
        private static readonly FileSystemWatcher s_EngineShaderWatcher;
        private static readonly ConcurrentQueue<FileSystemEventArgs> s_FileEvents = new();

        static ProjectWindow()
        {
            RefreshAssetsAndRebuildFileTree();

            s_AssetFileWatcher = new FileSystemWatcher(Path.Combine(Application.DataPath, "Assets"));
            s_EngineShaderWatcher = new FileSystemWatcher(Shader.EngineShaderPath);

            SetupFileSystemWatcher(s_AssetFileWatcher);
            SetupFileSystemWatcher(s_EngineShaderWatcher);
        }

        private static void SetupFileSystemWatcher(FileSystemWatcher watcher)
        {
            watcher.EnableRaisingEvents = true; // 必须设置为 true，否则事件不会触发
            watcher.IncludeSubdirectories = true;
            watcher.NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName | NotifyFilters.LastWrite;

            // 也可以用 watcher.SynchronizingObject
            watcher.Changed += (_, e) => s_FileEvents.Enqueue(e);
            watcher.Created += (_, e) => s_FileEvents.Enqueue(e);
            watcher.Deleted += (_, e) => s_FileEvents.Enqueue(e);
            watcher.Renamed += (_, e) => s_FileEvents.Enqueue(e);
        }

        private static void AddFileOrFolder(string fullPath, out string validatedPath)
        {
            validatedPath = AssetDatabase.GetValidatedAssetPathByFullPath(fullPath, out AssetPathType type);

            if (type == AssetPathType.ProjectAsset)
            {
                bool isFolder = File.GetAttributes(fullPath).HasFlag(FileAttributes.Directory);
                s_FileTree.Add(validatedPath, isFolder);
            }
        }

        private static void RemoveFileOrFolder(string fullPath)
        {
            string path = AssetDatabase.GetValidatedAssetPathByFullPath(fullPath, out AssetPathType type);

            if (type == AssetPathType.ProjectAsset)
            {
                s_FileTree.Remove(path);
            }
        }

        private static void OnFileChanged(FileSystemEventArgs e)
        {
            AssetDatabase.OnAssetChanged(e.FullPath);
        }

        private static void OnFileCreated(FileSystemEventArgs e)
        {
            AssetDatabase.OnAssetCreated(e.FullPath);
            AddFileOrFolder(e.FullPath, out _);
        }

        private static void OnFileDeleted(FileSystemEventArgs e)
        {
            AssetDatabase.OnAssetDeleted(e.FullPath);
            RemoveFileOrFolder(e.FullPath);
        }

        private static void OnFileRenamed(RenamedEventArgs e)
        {
            AssetDatabase.OnAssetRenamed(e.OldFullPath, e.FullPath, out bool isVisualStudioSavingFile);

            if (!isVisualStudioSavingFile)
            {
                RemoveFileOrFolder(e.OldFullPath);
                AddFileOrFolder(e.FullPath, out _);
            }
        }

        private static void RefreshAssetsAndRebuildFileTree()
        {
            s_FileTree.Clear();

            var root1 = new DirectoryInfo(Path.Combine(Application.DataPath, "Assets"));

            foreach (FileSystemInfo info in root1.EnumerateFileSystemInfos("*", SearchOption.AllDirectories))
            {
                AddFileOrFolder(info.FullName, out string validatedPath);
                AssetDatabase.GetAssetImporter(validatedPath);
            }

            var root2 = new DirectoryInfo(Shader.EngineShaderPath);

            foreach (FileSystemInfo info in root2.EnumerateFileSystemInfos("*", SearchOption.AllDirectories))
            {
                AddFileOrFolder(info.FullName, out string validatedPath);
                AssetDatabase.GetAssetImporter(validatedPath);
            }

            AssetDatabase.TrimGuidMap();
        }

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            while (s_FileEvents.TryDequeue(out FileSystemEventArgs? e))
            {
                switch (e.ChangeType)
                {
                    case WatcherChangeTypes.Changed:
                        //Debug.LogInfo("Changed: " + e.FullPath);
                        OnFileChanged(e);
                        break;

                    case WatcherChangeTypes.Created:
                        //Debug.LogInfo("Created: " + e.FullPath);
                        OnFileCreated(e);
                        break;

                    case WatcherChangeTypes.Deleted:
                        //Debug.LogInfo("Deleted: " + e.FullPath);
                        OnFileDeleted(e);
                        break;

                    case WatcherChangeTypes.Renamed:
                        //Debug.LogInfo($"Renamed: {((RenamedEventArgs)e).OldFullPath} -> {e.FullPath}");
                        OnFileRenamed((RenamedEventArgs)e);
                        break;
                }
            }

            s_FileTree.Draw();
        }
    }
}
