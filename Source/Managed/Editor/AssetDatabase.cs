using March.Core;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.Importers;
using System.Collections.Concurrent;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;

namespace March.Editor
{
    public static class AssetDatabase
    {
        private const string ImporterPathSuffix = ".meta";

        private static readonly Dictionary<string, Type> s_AssetImporterTypeMap = new();
        private static int? s_AssetImporterTypeCacheVersion;

        // TODO: use case-insensitive dictionary
        private static readonly Dictionary<string, string> s_Guid2PathMap = new();
        private static readonly Dictionary<string, AssetImporter> s_Path2Importers = new();

        private static readonly FileSystemWatcher s_ProjectAssetFileWatcher;
        private static readonly FileSystemWatcher s_EngineShaderWatcher;
        private static readonly ConcurrentQueue<FileSystemEventArgs> s_FileSystemEvents = new();

        public static event Action<string>? OnProjectAssetChanged;
        public static event Action<string>? OnProjectAssetImported;
        public static event Action<string>? OnProjectAssetRemoved;
        public static event Action<string, string>? OnProjectAssetRenamed;

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
                kv.Value.ReimportAssetIfNeeded();
            }

            static void CreateImportersOnly(string directoryFullPath)
            {
                var root = new DirectoryInfo(directoryFullPath);

                foreach (FileSystemInfo info in root.EnumerateFileSystemInfos("*", SearchOption.AllDirectories))
                {
                    string path = GetValidatedAssetPathByFullPath(info.FullName, out _);

                    // 仅创建 importer，不导入资产，构建一张 guid 到 importer 的表
                    // 资产间的依赖是用 guid 记录的，只有构建完 guid 表才能正确导入资产
                    GetAssetImporter(path, reimportAssetIfNeeded: false);
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
                if (IsImporterFilePath(e.FullPath))
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
            return importer?.AssetGuid;
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
            if (IsImporterFilePath(path))
            {
                throw new InvalidOperationException("Can not load an asset importer");
            }

            AssetImporter? importer = GetAssetImporter(path);
            return importer?.Asset as T;
        }

        public static MarchObject? LoadByGuid(string guid)
        {
            return LoadByGuid<MarchObject>(guid);
        }

        public static T? LoadByGuid<T>(string guid) where T : MarchObject?
        {
            string? path = GetPathByGuid(guid);
            return path == null ? null : Load<T>(path);
        }

        public static string[] GetAllProjectAssetPaths()
        {
            using var list = ListPool<string>.Get();

            foreach (KeyValuePair<string, AssetImporter> kv in s_Path2Importers)
            {
                if (kv.Value.Category == AssetCategory.ProjectAsset)
                {
                    list.Value.Add(kv.Value.AssetPath);
                }
            }

            return list.Value.ToArray();
        }

        public static void Create(string path, MarchObject asset)
        {
            if (asset.PersistentGuid != null || GetAssetImporter(path) != null)
            {
                throw new InvalidOperationException("Asset is already created");
            }

            AssetCategory category = ValidateAssetPathAndGetCategory(ref path, out string fullPath, out _);

            if (category != AssetCategory.ProjectAsset)
            {
                throw new InvalidOperationException("Can not create an asset outside project folder");
            }

            PersistentManager.Save(asset, fullPath);
            MustCreateAssetImporter(path, asset);

            if (category == AssetCategory.ProjectAsset)
            {
                OnProjectAssetImported?.Invoke(path);
            }
        }

        private static void OnAssetChanged(FileSystemEventArgs e)
        {
            //Debug.LogInfo("Changed: " + e.FullPath);

            string path = GetValidatedAssetPathByFullPath(e.FullPath, out AssetCategory category);

            if (category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to reimport an asset whose path is unknown: {e.FullPath}");
                return;
            }

            AssetImporter? importer = GetAssetImporter(path);

            if (importer != null && importer.NeedReimportAsset())
            {
                importer.SaveImporterAndReimportAsset();

                if (category == AssetCategory.ProjectAsset)
                {
                    OnProjectAssetChanged?.Invoke(path);
                }
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

            string oldPath = GetValidatedAssetPathByFullPath(e.OldFullPath, out AssetCategory oldCategory);
            string newPath = GetValidatedAssetPathByFullPath(e.FullPath, out AssetCategory newCategory);

            if (oldCategory == AssetCategory.Unknown || newCategory == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to rename an asset whose path is unknown: {e.OldFullPath} -> {e.FullPath}");
                return;
            }

            if (string.Equals(oldPath, newPath, StringComparison.CurrentCultureIgnoreCase))
            {
                return;
            }

            AssetImporter? oldImporter = GetAssetImporter(oldPath);

            if (oldImporter == null)
            {
                Debug.LogWarning("Attempting to rename an asset whose importer is unknown: " + oldPath);
                return;
            }

            MarchObject asset = oldImporter.Asset;
            DeleteImporter(oldImporter, false);

            if (s_Path2Importers.TryGetValue(newPath, out AssetImporter? newImporter))
            {
                Debug.LogWarning("Asset already exists at new path. It will be deleted.");
                AssetCategory assetCategory = newImporter.Category;
                DeleteImporter(newImporter, true);

                if (assetCategory == AssetCategory.ProjectAsset)
                {
                    OnProjectAssetRemoved?.Invoke(newPath);
                }
            }

            newImporter = MustCreateAssetImporter(newPath, asset);

            bool oldIsProjectAsset = oldCategory == AssetCategory.ProjectAsset;
            bool newIsProjectAsset = newCategory == AssetCategory.ProjectAsset;

            if (oldIsProjectAsset && newIsProjectAsset)
            {
                OnProjectAssetRenamed?.Invoke(oldPath, newPath);
            }
            else if (oldIsProjectAsset)
            {
                OnProjectAssetRemoved?.Invoke(oldPath);
            }
            else if (newIsProjectAsset)
            {
                OnProjectAssetImported?.Invoke(newPath);
            }
        }

        private static void OnAssetCreated(FileSystemEventArgs e)
        {
            //Debug.LogInfo("Created: " + e.FullPath);

            string path = GetValidatedAssetPathByFullPath(e.FullPath, out AssetCategory category);

            if (category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to create and import an asset whose path is unknown: {e.FullPath}");
                return;
            }

            if (s_Path2Importers.ContainsKey(path))
            {
                return;
            }

            AssetImporter? importer = GetAssetImporter(path);

            if (importer != null && category == AssetCategory.ProjectAsset)
            {
                OnProjectAssetImported?.Invoke(path);
            }
        }

        private static void OnAssetDeleted(FileSystemEventArgs e)
        {
            //Debug.LogInfo("Deleted: " + e.FullPath);

            string path = GetValidatedAssetPathByFullPath(e.FullPath, out AssetCategory category);

            if (category == AssetCategory.Unknown)
            {
                Debug.LogWarning($"Attempting to delete an asset whose path is unknown: {e.FullPath}");
                return;
            }

            AssetImporter? importer = GetAssetImporter(path);

            if (importer != null)
            {
                DeleteImporter(importer, true);

                if (category == AssetCategory.ProjectAsset)
                {
                    OnProjectAssetRemoved?.Invoke(path);
                }
            }
        }

        private static void DeleteImporter(AssetImporter importer, bool deleteAssetCache)
        {
            if (deleteAssetCache && File.Exists(importer.AssetCacheFullPath))
            {
                File.Delete(importer.AssetCacheFullPath);
            }

            if (File.Exists(importer.ImporterFullPath))
            {
                File.Delete(importer.ImporterFullPath);
            }

            s_Guid2PathMap.Remove(importer.AssetGuid);
            s_Path2Importers.Remove(importer.AssetPath);
        }

        internal static bool IsImporterFilePath(string path) => path.EndsWith(ImporterPathSuffix);

        public static bool IsFolder(string path) => IsFolder(GetAssetImporter(path));

        public static bool IsFolder([NotNullWhen(true)] AssetImporter? importer) => importer is FolderImporter;

        public static bool IsAsset(string path) => GetAssetImporter(path) != null;

        private static AssetCategory ValidateAssetPathAndGetCategory(ref string path, out string fullPath, out string importerFullPath)
        {
            path = path.ValidatePath();

            if (!IsImporterFilePath(path))
            {
                // 需要排除掉 Assets 文件夹本身
                if (path.StartsWith("Assets/", StringComparison.CurrentCultureIgnoreCase))
                {
                    fullPath = Path.Combine(Application.DataPath, path).ValidatePath();
                    importerFullPath = fullPath + ImporterPathSuffix;
                    return AssetCategory.ProjectAsset;
                }

                // 需要排除掉 Engine/Shaders 文件夹本身
                if (path.StartsWith("Engine/Shaders/", StringComparison.CurrentCultureIgnoreCase))
                {
                    // "Engine/".Length == 7
                    // "Engine/Shaders/".Length == 15
                    fullPath = Path.Combine(Shader.EngineShaderPath, path[15..]).ValidatePath();
                    importerFullPath = Path.Combine(Application.DataPath, "EngineMeta", path[7..] + ImporterPathSuffix).ValidatePath();
                    return AssetCategory.EngineShader;
                }
            }

            fullPath = path;
            importerFullPath = path + ImporterPathSuffix;
            return AssetCategory.Unknown;
        }

        public static AssetCategory GetAssetCategory(string path)
        {
            return ValidateAssetPathAndGetCategory(ref path, out _, out _);
        }

        public static string GetValidatedAssetPathByFullPath(string fullPath, out AssetCategory category)
        {
            fullPath = fullPath.ValidatePath();

            if (!IsImporterFilePath(fullPath))
            {
                if (fullPath.StartsWith(Application.DataPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    category = AssetCategory.ProjectAsset;
                    return fullPath[(Application.DataPath.Length + 1)..];
                }

                if (fullPath.StartsWith(Shader.EngineShaderPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    category = AssetCategory.EngineShader;
                    return "Engine/Shaders/" + fullPath[(Shader.EngineShaderPath.Length + 1)..];
                }
            }

            category = AssetCategory.Unknown;
            return fullPath;
        }

        public static AssetImporter? GetAssetImporter(string path, bool reimportAssetIfNeeded = true)
        {
            AssetImporter? importer = GetOrCreateAssetImporter(path, out bool isNewlyCreated);

            if (reimportAssetIfNeeded && importer != null && isNewlyCreated)
            {
                importer.ReimportAssetIfNeeded();
            }

            return importer;
        }

        private static AssetImporter MustCreateAssetImporter(string path, MarchObject? initAsset)
        {
            AssetImporter? importer = GetOrCreateAssetImporter(path, out bool isNewlyCreated);

            if (importer == null || !isNewlyCreated)
            {
                throw new Exception("Failed to create asset importer");
            }

            if (initAsset == null)
            {
                importer.ReimportAssetIfNeeded();
            }
            else
            {
                importer.SetAsset(initAsset);
            }

            return importer;
        }

        private static AssetImporter? GetOrCreateAssetImporter(string path, out bool isNewlyCreated)
        {
            AssetCategory category = ValidateAssetPathAndGetCategory(ref path, out string assetFullPath, out string importerFullPath);

            if (category == AssetCategory.Unknown)
            {
                isNewlyCreated = false;
                return null;
            }

            if (s_Path2Importers.TryGetValue(path, out AssetImporter? importer))
            {
                isNewlyCreated = false;
            }
            else
            {
                if (!File.Exists(assetFullPath) && !Directory.Exists(assetFullPath))
                {
                    isNewlyCreated = false;
                    return null;
                }

                if (File.Exists(importerFullPath))
                {
                    importer = PersistentManager.Load<AssetImporter>(importerFullPath);
                }
                else
                {
                    if (!TryCreateAssetImporter(assetFullPath, out importer))
                    {
                        isNewlyCreated = false;
                        return null;
                    }
                }

                isNewlyCreated = true;
                importer.Initialize(category, path, assetFullPath, importerFullPath);
                s_Path2Importers.Add(path, importer);
                s_Guid2PathMap[importer.AssetGuid] = path;
            }

            return importer;
        }

        private static bool TryCreateAssetImporter(string fullPath, [NotNullWhen(true)] out AssetImporter? importer)
        {
            RebuildAssetImporterTypeMapIfNeeded();

            // 文件夹需要特殊处理
            if (Directory.Exists(fullPath))
            {
                importer = new FolderImporter();
                return true;
            }

            string extension = Path.GetExtension(fullPath);

            if (!s_AssetImporterTypeMap.TryGetValue(extension, out Type? type))
            {
                importer = null;
                return false;
            }

            try
            {
                importer = Activator.CreateInstance(type) as AssetImporter;
                return importer != null;
            }
            catch
            {
                importer = null;
                return false;
            }
        }

        private static void RebuildAssetImporterTypeMapIfNeeded()
        {
            if (s_AssetImporterTypeCacheVersion == TypeCache.Version)
            {
                return;
            }

            s_AssetImporterTypeMap.Clear();

            foreach (Type type in TypeCache.GetTypesDerivedFrom<AssetImporter>())
            {
                var attr = type.GetCustomAttribute<CustomAssetImporterAttribute>();

                if (attr == null)
                {
                    continue;
                }

                foreach (string extension in attr.Extensions)
                {
                    s_AssetImporterTypeMap[extension] = type;
                }
            }

            s_AssetImporterTypeCacheVersion = TypeCache.Version;
        }

        [return: NotNullIfNotNull(nameof(path))]
        public static string? ValidatePath(this string? path)
        {
            if (path != null)
            {
                path = path.Replace('\\', '/');
                path = path.TrimEnd('/');
            }

            return path;
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
    }
}
