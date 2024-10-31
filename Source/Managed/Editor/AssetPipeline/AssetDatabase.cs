using March.Core;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.AssetPipeline.Importers;
using System.Collections.Concurrent;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor.AssetPipeline
{
    public static class AssetDatabase
    {
        private struct NativeReference
        {
            public NativeMarchObject Object;
            public int RefCount;
        }

        // TODO: use case-insensitive dictionary
        private static readonly Dictionary<string, string> s_Guid2PathMap = new();
        private static readonly Dictionary<string, AssetImporter> s_Path2Importers = new();

        // TODO: check memory leaks
        private static readonly Dictionary<nint, NativeReference> s_NativeRefs = new();

        private static readonly FileSystemWatcher s_ProjectAssetFileWatcher;
        private static readonly FileSystemWatcher s_EngineShaderWatcher;
        private static readonly ConcurrentQueue<FileSystemEventArgs> s_FileSystemEvents = new();

        public static event Action<AssetLocation>? OnChanged;
        public static event Action<AssetLocation>? OnImported;
        public static event Action<AssetLocation>? OnRemoved;
        public static event Action<AssetLocation, AssetLocation>? OnRenamed;

        static AssetDatabase()
        {
            s_ProjectAssetFileWatcher = new FileSystemWatcher(Path.Combine(Application.DataPath, "Assets"));
            s_EngineShaderWatcher = new FileSystemWatcher(Shader.EngineShaderPath);
        }

        internal static void Initialize()
        {
            PreImportAllAssets();

            SetupFileSystemWatcher(s_ProjectAssetFileWatcher);
            SetupFileSystemWatcher(s_EngineShaderWatcher);
        }

        private static void PreImportAllAssets()
        {
            CreateImportersOnly(Path.Combine(Application.DataPath, "Assets"));
            CreateImportersOnly(Shader.EngineShaderPath);

            // 现在 guid 表已经构建完毕，可以导入资产了
            AssetManager.Impl = new EditorAssetManagerImpl();

            foreach (KeyValuePair<string, AssetImporter> kv in s_Path2Importers)
            {
                kv.Value.ReimportAndSave(force: false);
            }

            static void CreateImportersOnly(string directoryFullPath)
            {
                var root = new DirectoryInfo(directoryFullPath);

                foreach (FileSystemInfo info in root.EnumerateFileSystemInfos("*", SearchOption.AllDirectories))
                {
                    var location = AssetLocation.FromFullPath(info.FullName);

                    // 仅创建 importer，不导入资产，构建一张 guid 到 importer 的表
                    // 资产间的依赖是用 guid 记录的，只有构建完 guid 表才能正确导入资产
                    GetAssetImporter(location.AssetPath, reimportAssetIfNeeded: false);
                }
            }
        }

        private static void SetupFileSystemWatcher(FileSystemWatcher watcher)
        {
            watcher.EnableRaisingEvents = true; // 必须设置为 true，否则事件不会触发
            watcher.IncludeSubdirectories = true;
            watcher.NotifyFilter = NotifyFilters.FileName | NotifyFilters.DirectoryName | NotifyFilters.LastWrite;

            // 也可以用 watcher.SynchronizingObject
            watcher.Changed += (_, e) => s_FileSystemEvents.Enqueue(e);
            watcher.Created += (_, e) => s_FileSystemEvents.Enqueue(e);
            watcher.Deleted += (_, e) => s_FileSystemEvents.Enqueue(e);
            watcher.Renamed += (_, e) => s_FileSystemEvents.Enqueue(e);
        }

        internal static void Update()
        {
            while (s_FileSystemEvents.TryDequeue(out FileSystemEventArgs? e))
            {
                if (AssetLocation.IsImporterFilePath(e.FullPath))
                {
                    continue;
                }

                switch (e.ChangeType)
                {
                    case WatcherChangeTypes.Changed:
                        OnAssetChanged(e);
                        break;

                    case WatcherChangeTypes.Created:
                        OnAssetCreated(e);
                        break;

                    case WatcherChangeTypes.Deleted:
                        OnAssetDeleted(e);
                        break;

                    case WatcherChangeTypes.Renamed:
                        OnAssetRenamed((RenamedEventArgs)e);
                        break;
                }
            }
        }

        internal static void Dispose()
        {
            s_ProjectAssetFileWatcher.Dispose();
            s_EngineShaderWatcher.Dispose();
        }

        public static string? GetGuidByPath(string path)
        {
            AssetImporter? importer = GetAssetImporter(path);
            return importer?.MainAssetGuid;
        }

        public static string? GetPathByGuid(string guid)
        {
            return s_Guid2PathMap.TryGetValue(guid, out string? path) ? path : null;
        }

        public static MarchObject? Load(string path)
        {
            return Load<MarchObject>(path);
        }

        public static T? Load<T>(string path) where T : MarchObject?
        {
            if (AssetLocation.IsImporterFilePath(path))
            {
                throw new InvalidOperationException("Can not load asset importer");
            }

            AssetImporter? importer = GetAssetImporter(path);
            importer?.ReimportAndSave(force: false); // 保证拿到新的资产
            return importer?.MainAsset as T;
        }

        public static MarchObject? LoadByGuid(string guid)
        {
            return LoadByGuid<MarchObject>(guid);
        }

        public static T? LoadByGuid<T>(string guid) where T : MarchObject?
        {
            if (!s_Guid2PathMap.TryGetValue(guid, out string? path))
            {
                return null;
            }

            AssetImporter? importer = GetAssetImporter(path);
            importer?.ReimportAndSave(force: false); // 保证拿到新的资产
            return importer?.GetAsset(guid) as T;
        }

        public static void GetAllAssetLocations(List<AssetLocation> locations)
        {
            foreach (KeyValuePair<string, AssetImporter> kv in s_Path2Importers)
            {
                locations.Add(kv.Value.Location);
            }
        }

        public static void Create(string path, MarchObject asset)
        {
            var location = AssetLocation.FromPath(path);

            if (asset.PersistentGuid != null || s_Path2Importers.ContainsKey(location.AssetPath))
            {
                throw new InvalidOperationException("Asset is already created");
            }

            if (location.Category != AssetCategory.ProjectAsset)
            {
                throw new InvalidOperationException("Can not create an asset outside project folder");
            }

            AssetImporter? importer = GetOrCreateAssetImporter(location.AssetPath, out bool isNewlyCreated);

            if (importer is not DirectAssetImporter directAssetImporter || !isNewlyCreated)
            {
                if (importer != null && isNewlyCreated)
                {
                    DeleteImporter(importer);
                }

                throw new InvalidOperationException("Can not create an asset at this path");
            }

            directAssetImporter.SetAssetAndSave(asset);
            RegisterAssetImporterGuids(directAssetImporter);
            AddImporterEventHandlers(directAssetImporter);

            // 通过 FileSystemWatcher 的 Create 事件触发 OnImported 事件
            // OnImported?.Invoke(location);
        }

        private static void OnAssetChanged(FileSystemEventArgs e)
        {
            //Debug.LogInfo("Changed: " + e.FullPath);

            var location = AssetLocation.FromFullPath(e.FullPath);

            if (location.Category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to reimport an asset whose path is unknown: {e.FullPath}");
                return;
            }

            AssetImporter? importer = GetAssetImporter(location.AssetPath);

            if (importer != null && importer.ReimportAndSave(force: false))
            {
                OnChanged?.Invoke(location);
            }
        }

        private static bool IsVisualStudioTempFile(string path)
        {
            if (path.EndsWith('~'))
            {
                return true;
            }

            string ext = Path.GetExtension(path);
            return string.Equals(ext, ".TMP", StringComparison.OrdinalIgnoreCase);
        }

        private static void OnAssetRenamed(RenamedEventArgs e)
        {
            // Visual Studio 2022 保存 XXXX 文件的逻辑：
            // 1. 创建 ZZZZ~ 文件
            // 2. 新内容保存到 ZZZZ~ 文件
            // 3. 创建 XXXX~YYYY.TMP
            // 4. 删除 XXXX~YYYY.TMP
            // 5. 重命名 XXXX 文件为 XXXX~YYYY.TMP
            // 6. 重命名 ZZZZ~ 文件为 XXXX
            // 7. 删除 XXXX~YYYY.TMP

            if (IsVisualStudioTempFile(e.FullPath))
            {
                return;
            }

            if (IsVisualStudioTempFile(e.OldFullPath))
            {
                OnAssetChanged(e);
                return;
            }

            //Debug.LogInfo($"Renamed: {e.OldFullPath} -> {e.FullPath}");

            var oldLocation = AssetLocation.FromFullPath(e.OldFullPath);
            var newLocation = AssetLocation.FromFullPath(e.FullPath);

            if (oldLocation.Category == AssetCategory.Unknown || newLocation.Category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to rename an asset whose path is unknown: {e.OldFullPath} -> {e.FullPath}");
                return;
            }

            if (string.Equals(oldLocation.AssetPath, newLocation.AssetPath, StringComparison.CurrentCultureIgnoreCase))
            {
                return;
            }

            AssetImporter? oldImporter = GetAssetImporter(oldLocation.AssetPath);

            if (oldImporter == null)
            {
                Debug.LogWarning("Attempting to rename an asset whose importer is unknown: " + oldLocation.AssetPath);
                return;
            }

            if (s_Path2Importers.TryGetValue(newLocation.AssetPath, out AssetImporter? newImporter))
            {
                Debug.LogWarning("Asset already exists at new path. It will be deleted.");
                DeleteImporter(newImporter);
                OnRemoved?.Invoke(newLocation);
            }

            oldImporter.MoveLocation(in newLocation);
            OnRenamed?.Invoke(oldLocation, newLocation);
        }

        private static void OnAssetCreated(FileSystemEventArgs e)
        {
            //Debug.LogInfo("Created: " + e.FullPath);

            var location = AssetLocation.FromFullPath(e.FullPath);

            if (location.Category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to create and import an asset whose path is unknown: {e.FullPath}");
                return;
            }

            if (GetAssetImporter(location.AssetPath) != null)
            {
                OnImported?.Invoke(location);
            }
        }

        private static void OnAssetDeleted(FileSystemEventArgs e)
        {
            //Debug.LogInfo("Deleted: " + e.FullPath);

            var location = AssetLocation.FromFullPath(e.FullPath);

            if (location.Category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to delete an asset whose path is unknown: {e.FullPath}");
                return;
            }

            AssetImporter? importer = GetAssetImporter(location.AssetPath);

            if (importer != null)
            {
                DeleteImporter(importer);
                OnRemoved?.Invoke(location);
            }
        }

        private static void DeleteImporter(AssetImporter importer)
        {
            importer.DeleteAssetCaches();
            importer.DeleteImporterFile();
            RemoveImporterEventHandlers(importer);
            UnregisterAssetImporterGuids(importer);
            s_Path2Importers.Remove(importer.Location.AssetPath);
        }

        public static bool IsFolder(string path) => IsFolder(GetAssetImporter(path));

        public static bool IsFolder([NotNullWhen(true)] AssetImporter? importer) => importer is FolderImporter;

        public static bool IsAsset(string path) => GetAssetImporter(path) != null;

        public static AssetImporter? GetAssetImporter(string path, bool reimportAssetIfNeeded = true)
        {
            AssetImporter? importer = GetOrCreateAssetImporter(path, out bool isNewlyCreated);

            if (importer != null && isNewlyCreated)
            {
                if (reimportAssetIfNeeded)
                {
                    importer.ReimportAndSave(force: false);
                }

                RegisterAssetImporterGuids(importer);
                AddImporterEventHandlers(importer);
            }

            return importer;
        }

        private static void AddImporterEventHandlers(AssetImporter importer)
        {
            importer.OnWillReimport += UnregisterAssetImporterGuids;
            importer.OnDidReimport += RegisterAssetImporterGuids;
        }

        private static void RemoveImporterEventHandlers(AssetImporter importer)
        {
            importer.OnWillReimport -= UnregisterAssetImporterGuids;
            importer.OnDidReimport -= RegisterAssetImporterGuids;
        }

        private static void UnregisterAssetImporterGuids(AssetImporter importer)
        {
            using var guids = ListPool<string>.Get();
            importer.GetAssetGuids(guids);

            foreach (string guid in guids.Value)
            {
                s_Guid2PathMap.Remove(guid);
            }
        }

        private static void RegisterAssetImporterGuids(AssetImporter importer)
        {
            using var guids = ListPool<string>.Get();
            importer.GetAssetGuids(guids);

            foreach (string guid in guids.Value)
            {
                s_Guid2PathMap.Add(guid, importer.Location.AssetPath);
            }
        }

        private static AssetImporter? GetOrCreateAssetImporter(string path, out bool isNewlyCreated)
        {
            var location = AssetLocation.FromPath(path);

            if (location.Category == AssetCategory.Unknown)
            {
                isNewlyCreated = false;
                return null;
            }

            if (s_Path2Importers.TryGetValue(location.AssetPath, out AssetImporter? importer))
            {
                isNewlyCreated = false;
            }
            else
            {
                // 允许给当前不存在的资产创建 DirectAssetImporter

                //if (!File.Exists(location.AssetFullPath) && !Directory.Exists(location.AssetFullPath))
                //{
                //    isNewlyCreated = false;
                //    return null;
                //}

                if (File.Exists(location.ImporterFullPath))
                {
                    importer = PersistentManager.Load<AssetImporter>(location.ImporterFullPath);
                }
                else
                {
                    importer = AssetImporter.CreateForPath(location.AssetFullPath);

                    if (importer == null)
                    {
                        isNewlyCreated = false;
                        return null;
                    }
                }

                isNewlyCreated = true;
                importer.InitLocation(in location);
                s_Path2Importers.Add(location.AssetPath, importer);
            }

            return importer;
        }

        internal static nint NativeLoadAsset(string path)
        {
            NativeMarchObject? obj = Load<NativeMarchObject>(path);

            if (obj == null)
            {
                Debug.LogError($"Attempting to load a non-native asset: {path}");
                return nint.Zero;
            }

            NativeReference nativeRefs = s_NativeRefs.GetValueOrDefault(obj.NativePtr);
            nativeRefs.Object = obj;
            nativeRefs.RefCount++;
            s_NativeRefs[obj.NativePtr] = nativeRefs;

            return obj.NativePtr;
        }

        internal static void NativeUnloadAsset(nint nativePtr)
        {
            if (!s_NativeRefs.TryGetValue(nativePtr, out NativeReference nativeRefs))
            {
                Debug.LogError("Attempting to unload an asset that is not loaded");
                return;
            }

            nativeRefs.RefCount--;

            if (nativeRefs.RefCount <= 0)
            {
                s_NativeRefs.Remove(nativePtr);
            }
            else
            {
                s_NativeRefs[nativePtr] = nativeRefs;
            }
        }
    }

    file class EditorAssetManagerImpl : IAssetManagerImpl
    {
        public string? GetGuidByPath(string path)
        {
            return AssetDatabase.GetGuidByPath(path);
        }

        public string? GetPathByGuid(string guid)
        {
            return AssetDatabase.GetPathByGuid(guid);
        }

        public T? Load<T>(string path) where T : MarchObject?
        {
            return AssetDatabase.Load<T>(path);
        }

        public T? LoadByGuid<T>(string guid) where T : MarchObject?
        {
            return AssetDatabase.LoadByGuid<T>(guid);
        }

        public nint NativeLoadAsset(string path)
        {
            return AssetDatabase.NativeLoadAsset(path);
        }

        public void NativeUnloadAsset(nint nativePtr)
        {
            AssetDatabase.NativeUnloadAsset(nativePtr);
        }
    }
}
