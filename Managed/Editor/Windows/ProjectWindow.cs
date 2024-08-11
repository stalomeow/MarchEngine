using DX12Demo.Core;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor.Windows
{
    internal static class ProjectWindow
    {
        private static readonly ProjectFileTree s_FileTree = new();
        private static readonly FileSystemWatcher s_FileWatcher;

        static ProjectWindow()
        {
            RebuildFileTree();

            s_FileWatcher = new FileSystemWatcher(Application.DataPath)
            {
                EnableRaisingEvents = true, // 必须设置为 true，否则事件不会触发
                IncludeSubdirectories = true,
                NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName | NotifyFilters.LastWrite,
            };
            s_FileWatcher.Changed += OnFileChanged;
            s_FileWatcher.Created += OnFileCreated;
            s_FileWatcher.Deleted += OnFileDeleted;
            s_FileWatcher.Renamed += OnFileRenamed;
        }

        private static void OnFileChanged(object sender, FileSystemEventArgs e)
        {
            Debug.LogWarning($"File changed: {e.FullPath}, {e.ChangeType}");
        }

        private static void OnFileCreated(object sender, FileSystemEventArgs e)
        {
            string path = GetRootRelativePath(e.FullPath);
            bool isFolder = File.GetAttributes(e.FullPath).HasFlag(FileAttributes.Directory);
            s_FileTree.Add(path, isFolder);
        }

        private static void OnFileDeleted(object sender, FileSystemEventArgs e)
        {
            string path = GetRootRelativePath(e.FullPath);
            s_FileTree.Remove(path);
        }

        private static void OnFileRenamed(object sender, RenamedEventArgs e)
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
                    case DirectoryInfo directory:
                        s_FileTree.AddFolder(GetRootRelativePath(directory.FullName));
                        break;

                    case FileInfo file:
                        s_FileTree.AddFile(GetRootRelativePath(file.FullName));
                        break;
                }
            }
        }

        private static string GetRootRelativePath(string path)
        {
            return path[(Application.DataPath.Length + 1)..];
        }

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            s_FileTree.Draw();
        }
    }
}
