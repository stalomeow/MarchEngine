using DX12Demo.Core;
using System.Collections.Concurrent;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor.Windows
{
    internal static class ProjectWindow
    {
        private static readonly ProjectFileTree s_FileTree = new();
        private static readonly FileSystemWatcher s_FileWatcher;
        private static readonly ConcurrentQueue<FileSystemEventArgs> s_FileEvents = new();

        static ProjectWindow()
        {
            RebuildFileTree();

            s_FileWatcher = new FileSystemWatcher(Application.DataPath)
            {
                EnableRaisingEvents = true, // 必须设置为 true，否则事件不会触发
                IncludeSubdirectories = true,
                NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName | NotifyFilters.LastWrite,
            };

            // 也可以用 s_FileWatcher.SynchronizingObject
            s_FileWatcher.Changed += (_, e) => s_FileEvents.Enqueue(e);
            s_FileWatcher.Created += (_, e) => s_FileEvents.Enqueue(e);
            s_FileWatcher.Deleted += (_, e) => s_FileEvents.Enqueue(e);
            s_FileWatcher.Renamed += (_, e) => s_FileEvents.Enqueue(e);
        }

        private static void OnFileChanged(FileSystemEventArgs e)
        {
            Debug.LogWarning($"File changed: {e.FullPath}, {e.ChangeType}");

            string path = GetRootRelativePath(e.FullPath).ValidatePath();
            AssetDatabase.GetAssetImporter(path)?.SaveAndReimport();
        }

        private static void OnFileCreated(FileSystemEventArgs e)
        {
            string path = GetRootRelativePath(e.FullPath);
            bool isFolder = File.GetAttributes(e.FullPath).HasFlag(FileAttributes.Directory);
            s_FileTree.Add(path, isFolder);
        }

        private static void OnFileDeleted(FileSystemEventArgs e)
        {
            string path = GetRootRelativePath(e.FullPath);
            s_FileTree.Remove(path);
        }

        private static void OnFileRenamed(RenamedEventArgs e)
        {
            s_FileTree.Remove(GetRootRelativePath(e.OldFullPath));

            string path = GetRootRelativePath(e.FullPath);
            bool isFolder = File.GetAttributes(e.FullPath).HasFlag(FileAttributes.Directory);
            s_FileTree.Add(path, isFolder);
        }

        private static void RebuildFileTree()
        {
            s_FileTree.Clear();

            var root = new DirectoryInfo(Application.DataPath);

            foreach (FileSystemInfo info in root.EnumerateFileSystemInfos("*", SearchOption.AllDirectories))
            {
                switch (info)
                {
                    case DirectoryInfo:
                        s_FileTree.AddFolder(GetRootRelativePath(info.FullName));
                        break;

                    case FileInfo:
                        s_FileTree.AddFile(GetRootRelativePath(info.FullName));
                        break;
                }

                AssetDatabase.GetAssetImporter(GetRootRelativePath(info.FullName).ValidatePath());
            }
        }

        private static string GetRootRelativePath(string path)
        {
            return path[(Application.DataPath.Length + 1)..];
        }

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            while (s_FileEvents.TryDequeue(out FileSystemEventArgs? e))
            {
                if (AssetDatabase.IsImporterFilePath(e.FullPath))
                {
                    continue;
                }

                switch (e.ChangeType)
                {
                    case WatcherChangeTypes.Changed:
                        OnFileChanged(e);
                        break;

                    case WatcherChangeTypes.Created:
                        OnFileCreated( e);
                        break;

                    case WatcherChangeTypes.Deleted:
                        OnFileDeleted(e);
                        break;

                    case WatcherChangeTypes.Renamed:
                        OnFileRenamed((RenamedEventArgs)e);
                        break;
                }
            }

            s_FileTree.Draw();
        }
    }
}
