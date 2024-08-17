using DX12Demo.Core;
using DX12Demo.Core.Serialization;
using DX12Demo.Editor.Importers;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;

namespace DX12Demo.Editor
{
    public static class AssetDatabase
    {
        private const string ImporterPathSuffix = ".meta";

        private static readonly Dictionary<string, Type> s_AssetImporterTypeMap = new();
        private static int? s_AssetImporterTypeCacheVersion;

        private static readonly Dictionary<string, AssetImporter> s_Importers = new();

        public static AssetHandle<EngineObject> Load(string path)
        {
            return Load<EngineObject>(path);
        }

        public static AssetHandle<T> Load<T>(string path) where T : EngineObject
        {
            if (IsImporterFilePath(path))
            {
                throw new FileNotFoundException("Can not find asset", path);
            }

            AssetImporter? importer = GetAssetImporter(path);
            importer?.IncreaseAssetRef();
            return new AssetHandle<T>(importer);
        }

        public static void Unload<T>(AssetHandle<T> handle) where T : EngineObject
        {
            if (handle.GetProvider() is not AssetImporter importer)
            {
                throw new ArgumentException($"Invalid {nameof(AssetHandle<T>)}", nameof(handle));
            }

            importer.DecreaseAssetRef();
        }

        public static void Save(EngineObject obj, string path)
        {
            string fullPath = Path.Combine(Application.DataPath, path);
            PersistentManager.Save(obj, fullPath);
        }

        internal static bool IsImporterFilePath(string path) => path.EndsWith(ImporterPathSuffix);

        public static bool IsFolder(string path) => IsFolder(GetAssetImporter(path));

        public static bool IsFolder([NotNullWhen(true)] AssetImporter? importer) => importer is FolderImporter;

        public static AssetImporter? GetAssetImporter(string path)
        {
            path = path.ValidatePath();

            if (!s_Importers.TryGetValue(path, out AssetImporter? importer))
            {
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
                    assetCacheFullPath: GetCacheFullPath(path).ValidatePath(),
                    importerFullPath: importerFullPath.ValidatePath());
                s_Importers.Add(path, importer);
            }

            return importer;

            static string GetCacheFullPath(string assetPath)
            {
                string path = Path.GetRelativePath("Assets", assetPath);
                return Path.Combine(Application.DataPath, "Library/AssetCache", path + ".cache");
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

        public static string ValidatePath(this string path) => path.Replace('\\', '/');
    }
}
