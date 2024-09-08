using DX12Demo.Core;
using DX12Demo.Core.Serialization;
using DX12Demo.Editor.Importers;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Text;

namespace DX12Demo.Editor
{
    public static class AssetDatabase
    {
        private const string ImporterPathSuffix = ".meta";

        private static readonly Dictionary<string, Type> s_AssetImporterTypeMap = new();
        private static int? s_AssetImporterTypeCacheVersion;

        private static readonly Dictionary<string, string> s_Guid2PathMap = new();
        private static readonly Dictionary<string, AssetImporter> s_Path2Importers = new();

        [ModuleInitializer]
        [SuppressMessage("Usage", "CA2255:The 'ModuleInitializer' attribute should not be used in libraries", Justification = "<Pending>")]
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

        public static EngineObject? Load(string path)
        {
            return Load<EngineObject>(path);
        }

        public static T? Load<T>(string path) where T : EngineObject?
        {
            if (IsImporterFilePath(path))
            {
                throw new InvalidOperationException("Can not load an asset importer");
            }

            AssetImporter? importer = GetAssetImporter(path);
            return importer?.Asset as T;
        }

        public static EngineObject? LoadByGuid(string guid)
        {
            return LoadByGuid<EngineObject>(guid);
        }

        public static T? LoadByGuid<T>(string guid) where T : EngineObject?
        {
            string? path = GetPathByGuid(guid);
            return path == null ? null : Load<T>(path);
        }

        public static void Create(string path, EngineObject asset)
        {
            if (asset.PersistentGuid != null || GetAssetImporter(path) != null)
            {
                throw new InvalidOperationException("Asset is already created");
            }

            string fullPath = Path.Combine(Application.DataPath, path);
            PersistentManager.Save(asset, fullPath);
            GetAssetImporter(path, asset); // Create importer
        }

        internal static bool IsImporterFilePath(string path) => path.EndsWith(ImporterPathSuffix);

        public static bool IsFolder(string path) => IsFolder(GetAssetImporter(path));

        public static bool IsFolder([NotNullWhen(true)] AssetImporter? importer) => importer is FolderImporter;

        public static bool IsAsset(string path) => GetAssetImporter(path) != null;

        public static AssetImporter? GetAssetImporter(string path)
        {
            return GetAssetImporter(path, null);
        }

        private static AssetImporter? GetAssetImporter(string path, EngineObject? initAsset)
        {
            path = path.ValidatePath();

            if (string.IsNullOrWhiteSpace(path) || path.Equals("assets", StringComparison.CurrentCultureIgnoreCase))
            {
                return null;
            }

            if (!s_Path2Importers.TryGetValue(path, out AssetImporter? importer))
            {
                string assetFullPath = Path.Combine(Application.DataPath, path);

                if (!File.Exists(assetFullPath) && !Directory.Exists(assetFullPath))
                {
                    return null;
                }

                string importerFullPath = Path.Combine(Application.DataPath, path + ImporterPathSuffix);

                if (File.Exists(importerFullPath))
                {
                    importer = PersistentManager.Load<AssetImporter>(importerFullPath);
                }
                else
                {
                    if (!TryCreateAssetImporter(path, out importer))
                    {
                        return null;
                    }
                }

                importer.Initialize(
                    assetPath: path.ValidatePath(),
                    assetFullPath: Path.Combine(Application.DataPath, path).ValidatePath(),
                    assetCacheFullPath: GetCacheFullPath(importer.AssetGuid).ValidatePath(),
                    importerFullPath: importerFullPath.ValidatePath(),
                    asset: initAsset);
                s_Path2Importers.Add(path, importer);
                s_Guid2PathMap[importer.AssetGuid] = path;
                SaveGuidMap(); // TODO: 换到其他地方 Save，避免频繁写入
            }

            return importer;

            static string GetCacheFullPath(string guid)
            {
                return Path.Combine(Application.DataPath, "Library/AssetCache", guid);
            }
        }

        private static bool TryCreateAssetImporter(string path, [NotNullWhen(true)] out AssetImporter? importer)
        {
            RebuildAssetImporterTypeMapIfNeeded();

            string fullPath = Path.Combine(Application.DataPath, path);

            // 文件夹需要特殊处理
            if (Directory.Exists(fullPath))
            {
                importer = new FolderImporter();
                return true;
            }

            string extension = Path.GetExtension(path);

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

        public T? Load<T>(string path) where T : EngineObject?
        {
            return AssetDatabase.Load<T>(path);
        }

        public T? LoadByGuid<T>(string guid) where T : EngineObject?
        {
            return AssetDatabase.LoadByGuid<T>(guid);
        }
    }
}
