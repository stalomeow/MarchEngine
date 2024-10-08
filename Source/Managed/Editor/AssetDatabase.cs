using March.Core;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.Importers;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Text;

namespace March.Editor
{
    public enum AssetPathType
    {
        ProjectAsset,
        EngineShader,
        Unknown,
    }

    public static class AssetDatabase
    {
        private const string ImporterPathSuffix = ".meta";

        private static readonly Dictionary<string, Type> s_AssetImporterTypeMap = new();
        private static int? s_AssetImporterTypeCacheVersion;

        // TODO: use case-insensitive dictionary
        private static readonly Dictionary<string, string> s_Guid2PathMap = new();
        private static readonly Dictionary<string, AssetImporter> s_Path2Importers = new();

        internal static void InitAssetDatabase()
        {
            AssetManager.Impl = new EditorAssetManagerImpl();
            LoadGuidMap(); // 提前加载缓存的 guid 表，这样后面 import asset 时，LoadByGuid 就能找到对应的 path
        }

        private static void LoadGuidMap()
        {
            string fullPath = GetGuidMapFullPath();

            if (File.Exists(fullPath))
            {
                string json = File.ReadAllText(fullPath, Encoding.UTF8);
                Dictionary<string, string>? data = JsonConvert.DeserializeObject<Dictionary<string, string>>(json);

                if (data != null)
                {
                    foreach (KeyValuePair<string, string> kvp in data)
                    {
                        s_Guid2PathMap[kvp.Key] = kvp.Value;
                    }

                    Debug.LogInfo($"AssetDatabase loads guid map with {data.Count} entries");
                }
            }
        }

        private static void SaveGuidMap()
        {
            string fullPath = GetGuidMapFullPath();
            string json = JsonConvert.SerializeObject(s_Guid2PathMap);
            File.WriteAllText(fullPath, json, Encoding.UTF8);
        }

        internal static void TrimGuidMap()
        {
            using var unusedGuids = ListPool<string>.Get();

            foreach (KeyValuePair<string, string> kv in s_Guid2PathMap)
            {
                if (!s_Path2Importers.ContainsKey(kv.Value))
                {
                    unusedGuids.Value.Add(kv.Key);
                }
            }

            foreach (string guid in unusedGuids.Value)
            {
                s_Guid2PathMap.Remove(guid);
            }

            SaveGuidMap(); // TODO: 换到其他地方 Save，避免频繁写入
        }

        private static string GetGuidMapFullPath() => Path.Combine(Application.DataPath, "Library/AssetGuidMap.json");

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

        public static void Create(string path, MarchObject asset)
        {
            if (asset.PersistentGuid != null || GetAssetImporter(path) != null)
            {
                throw new InvalidOperationException("Asset is already created");
            }

            if (ValidateAssetPathAndGetType(ref path, out string fullPath, out _) != AssetPathType.ProjectAsset)
            {
                throw new InvalidOperationException("Can not create an asset outside project folder");
            }

            PersistentManager.Save(asset, fullPath);
            GetAssetImporter(path, asset, null); // Create importer
        }

        internal static void OnAssetChanged(string fullPath)
        {
            string path = GetValidatedAssetPathByFullPath(fullPath, out AssetPathType pathType);

            if (pathType == AssetPathType.Unknown)
            {
                Debug.LogWarning($"Attempting to reimport an asset whose path is unknown: {fullPath}");
                return;
            }

            AssetImporter? importer = GetAssetImporter(path);

            if (importer != null && importer.NeedReimportAsset())
            {
                importer.SaveImporterAndReimportAsset();
            }
        }

        private static bool IsVisualStudioTempSavingFile(string path)
        {
            if (path.EndsWith('~'))
            {
                return true;
            }

            string ext = Path.GetExtension(path);
            return string.Equals(ext, ".TMP", StringComparison.OrdinalIgnoreCase);
        }

        internal static void OnAssetRenamed(string oldFullPath, string newFullPath, out bool isVisualStudioSavingFile)
        {
            // Visual Studio 2022 保存 XXXX 文件的逻辑：
            // 1. 创建 ZZZZ~ 文件
            // 2. 新内容保存到 ZZZZ~ 文件
            // 3. 创建 XXXX~YYYY.TMP
            // 4. 删除 XXXX~YYYY.TMP
            // 5. 重命名 XXXX 文件为 XXXX~YYYY.TMP
            // 6. 重命名 ZZZZ~ 文件为 XXXX
            // 7. 删除 XXXX~YYYY.TMP

            if (IsVisualStudioTempSavingFile(newFullPath))
            {
                isVisualStudioSavingFile = true;
                return;
            }

            if (IsVisualStudioTempSavingFile(oldFullPath))
            {
                isVisualStudioSavingFile = true;
                OnAssetChanged(newFullPath);
                return;
            }

            isVisualStudioSavingFile = false;
            string oldPath = GetValidatedAssetPathByFullPath(oldFullPath, out AssetPathType oldPathType);
            string newPath = GetValidatedAssetPathByFullPath(newFullPath, out AssetPathType newPathType);

            if (oldPathType == AssetPathType.Unknown || newPathType == AssetPathType.Unknown)
            {
                Debug.LogWarning($"Attempting to rename an asset whose path is unknown: {oldFullPath} -> {newFullPath}");
                return;
            }

            if (string.Equals(oldPath, newPath, StringComparison.CurrentCultureIgnoreCase))
            {
                return;
            }

            AssetImporter oldImporter = GetAssetImporter(oldPath) ?? throw new ArgumentException("Old asset not found");
            MarchObject asset = oldImporter.Asset;
            string assetGuid = oldImporter.AssetGuid;
            DeleteImporter(oldImporter, false, false);

            if (s_Path2Importers.TryGetValue(newPath, out AssetImporter? newImporter))
            {
                Debug.LogWarning("Asset already exists at new path. It will be deleted.");
                DeleteImporter(newImporter, true, false);
            }

            // Recreate importer; GuidMap will be saved in this method
            GetAssetImporter(newPath, asset, assetGuid);
        }

        internal static void OnAssetCreated(string fullPath)
        {
            string path = GetValidatedAssetPathByFullPath(fullPath, out AssetPathType pathType);

            if (pathType == AssetPathType.Unknown)
            {
                Debug.LogWarning($"Attempting to create and import an asset whose path is unknown: {fullPath}");
                return;
            }

            GetAssetImporter(path);
        }

        internal static void OnAssetDeleted(string fullPath)
        {
            string path = GetValidatedAssetPathByFullPath(fullPath, out AssetPathType pathType);

            if (pathType == AssetPathType.Unknown)
            {
                Debug.LogWarning($"Attempting to delete an asset whose path is unknown: {fullPath}");
                return;
            }

            AssetImporter? importer = GetAssetImporter(path);

            if (importer != null)
            {
                DeleteImporter(importer, true, true);
            }
        }

        private static void DeleteImporter(AssetImporter importer, bool deleteAssetCache, bool saveGuidMap)
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

            if (saveGuidMap)
            {
                SaveGuidMap(); // TODO: 换到其他地方 Save，避免频繁写入
            }
        }

        internal static bool IsImporterFilePath(string path) => path.EndsWith(ImporterPathSuffix);

        public static bool IsFolder(string path) => IsFolder(GetAssetImporter(path));

        public static bool IsFolder([NotNullWhen(true)] AssetImporter? importer) => importer is FolderImporter;

        public static bool IsAsset(string path) => GetAssetImporter(path) != null;

        private static AssetPathType ValidateAssetPathAndGetType(ref string path, out string fullPath, out string importerFullPath)
        {
            path = path.ValidatePath();

            if (!IsImporterFilePath(path))
            {
                // 需要排除掉 Assets 文件夹本身
                if (path.StartsWith("Assets/", StringComparison.CurrentCultureIgnoreCase))
                {
                    fullPath = Path.Combine(Application.DataPath, path).ValidatePath();
                    importerFullPath = fullPath + ImporterPathSuffix;
                    return AssetPathType.ProjectAsset;
                }

                // 需要排除掉 Engine/Shaders 文件夹本身
                if (path.StartsWith("Engine/Shaders/", StringComparison.CurrentCultureIgnoreCase))
                {
                    // "Engine/".Length == 7
                    // "Engine/Shaders/".Length == 15
                    fullPath = Path.Combine(Shader.EngineShaderPath, path[15..]).ValidatePath();
                    importerFullPath = Path.Combine(Application.DataPath, "EngineMeta", path[7..] + ImporterPathSuffix).ValidatePath();
                    return AssetPathType.EngineShader;
                }
            }

            fullPath = path;
            importerFullPath = path + ImporterPathSuffix;
            return AssetPathType.Unknown;
        }

        public static AssetPathType GetAssetPathType(string path)
        {
            return ValidateAssetPathAndGetType(ref path, out _, out _);
        }

        public static string GetValidatedAssetPathByFullPath(string fullPath, out AssetPathType type)
        {
            fullPath = fullPath.ValidatePath();

            if (!IsImporterFilePath(fullPath))
            {
                if (fullPath.StartsWith(Application.DataPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    type = AssetPathType.ProjectAsset;
                    return fullPath[(Application.DataPath.Length + 1)..];
                }

                if (fullPath.StartsWith(Shader.EngineShaderPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    type = AssetPathType.EngineShader;
                    return "Engine/Shaders/" + fullPath[(Shader.EngineShaderPath.Length + 1)..];
                }
            }

            type = AssetPathType.Unknown;
            return fullPath;
        }

        public static AssetImporter? GetAssetImporter(string path)
        {
            return GetAssetImporter(path, null, null);
        }

        private static AssetImporter? GetAssetImporter(string path, MarchObject? initAsset, string? initAssetGuid)
        {
            AssetPathType pathType = ValidateAssetPathAndGetType(ref path, out string assetFullPath, out string importerFullPath);

            if (pathType == AssetPathType.Unknown)
            {
                return null;
            }

            if (!s_Path2Importers.TryGetValue(path, out AssetImporter? importer))
            {
                if (!File.Exists(assetFullPath) && !Directory.Exists(assetFullPath))
                {
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
                        return null;
                    }
                }

                importer.Initialize(
                    assetPath: path,
                    assetFullPath: assetFullPath,
                    assetCacheFullPath: GetAssetCacheFullPath(importer.AssetGuid),
                    importerFullPath: importerFullPath,
                    asset: initAsset,
                    assetGuid: initAssetGuid);
                s_Path2Importers.Add(path, importer);
                s_Guid2PathMap[importer.AssetGuid] = path;
                SaveGuidMap(); // TODO: 换到其他地方 Save，避免频繁写入
            }

            return importer;
        }

        private static string GetAssetCacheFullPath(string guid)
        {
            return Path.Combine(Application.DataPath, "Library/AssetCache", guid).ValidatePath();
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
