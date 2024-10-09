using March.Core;
using March.Core.Rendering;
using System.Collections.Concurrent;

namespace March.Editor.Windows
{
    internal class ProjectWindow : EditorWindow
    {
        private readonly ProjectFileTree m_FileTree = new();
        private readonly FileSystemWatcher m_AssetFileWatcher;
        private readonly FileSystemWatcher m_EngineShaderWatcher;
        private readonly ConcurrentQueue<FileSystemEventArgs> m_FileEvents = new();

        public ProjectWindow() : base("Project")
        {
            RefreshAssetsAndRebuildFileTree();

            m_AssetFileWatcher = new FileSystemWatcher(Path.Combine(Application.DataPath, "Assets"));
            m_EngineShaderWatcher = new FileSystemWatcher(Shader.EngineShaderPath);

            SetupFileSystemWatcher(m_AssetFileWatcher);
            SetupFileSystemWatcher(m_EngineShaderWatcher);
        }

        protected override void OnDispose(bool disposing)
        {
            m_AssetFileWatcher.Dispose();
            m_EngineShaderWatcher.Dispose();

            base.OnDispose(disposing);
        }

        private void SetupFileSystemWatcher(FileSystemWatcher watcher)
        {
            watcher.EnableRaisingEvents = true; // 必须设置为 true，否则事件不会触发
            watcher.IncludeSubdirectories = true;
            watcher.NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName | NotifyFilters.LastWrite;

            // 也可以用 watcher.SynchronizingObject
            watcher.Changed += (_, e) => m_FileEvents.Enqueue(e);
            watcher.Created += (_, e) => m_FileEvents.Enqueue(e);
            watcher.Deleted += (_, e) => m_FileEvents.Enqueue(e);
            watcher.Renamed += (_, e) => m_FileEvents.Enqueue(e);
        }

        private void AddFileOrFolder(string fullPath, out string validatedPath)
        {
            validatedPath = AssetDatabase.GetValidatedAssetPathByFullPath(fullPath, out AssetPathType type);

            if (type == AssetPathType.ProjectAsset)
            {
                bool isFolder = File.GetAttributes(fullPath).HasFlag(FileAttributes.Directory);
                m_FileTree.Add(validatedPath, isFolder);
            }
        }

        private void RemoveFileOrFolder(string fullPath)
        {
            string path = AssetDatabase.GetValidatedAssetPathByFullPath(fullPath, out AssetPathType type);

            if (type == AssetPathType.ProjectAsset)
            {
                m_FileTree.Remove(path);
            }
        }

        private void OnFileChanged(FileSystemEventArgs e)
        {
            AssetDatabase.OnAssetChanged(e.FullPath);
        }

        private void OnFileCreated(FileSystemEventArgs e)
        {
            AssetDatabase.OnAssetCreated(e.FullPath);
            AddFileOrFolder(e.FullPath, out _);
        }

        private void OnFileDeleted(FileSystemEventArgs e)
        {
            AssetDatabase.OnAssetDeleted(e.FullPath);
            RemoveFileOrFolder(e.FullPath);
        }

        private void OnFileRenamed(RenamedEventArgs e)
        {
            AssetDatabase.OnAssetRenamed(e.OldFullPath, e.FullPath, out bool isVisualStudioSavingFile);

            if (!isVisualStudioSavingFile)
            {
                RemoveFileOrFolder(e.OldFullPath);
                AddFileOrFolder(e.FullPath, out _);
            }
        }

        private void RefreshAssetsAndRebuildFileTree()
        {
            m_FileTree.Clear();

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

        protected override void OnDraw()
        {
            base.OnDraw();

            while (m_FileEvents.TryDequeue(out FileSystemEventArgs? e))
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

            m_FileTree.Draw();
        }
    }
}
