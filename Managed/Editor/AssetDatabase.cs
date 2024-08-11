using DX12Demo.Core;
using DX12Demo.Editor.Importers;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace DX12Demo.Editor
{
    public static class AssetDatabase
    {
        private static readonly Dictionary<string, Type> s_AssetImporterTypeMap = new();
        private static int? s_AssetImporterTypeCacheVersion;

        private static readonly Dictionary<string, WeakReference<EngineObject>> s_LoadedObjects = new();
        private static readonly Dictionary<string, WeakReference<AssetImporter>> s_LoadedImporters = new(); // key 为原 asset 的路径
        private static readonly ConditionalWeakTable<EngineObject, string> s_ObjectFullPath = new();
        private static readonly HashSet<EngineObject> s_DirtyObjects = new();

        public static EngineObject LoadAtPath(string path)
        {
            return LoadAtPath<EngineObject>(path);
        }

        public static T LoadAtPath<T>(string path) where T : EngineObject
        {
            if (s_LoadedObjects.TryGetValue(path, out WeakReference<EngineObject>? weakRef))
            {
                if (weakRef.TryGetTarget(out EngineObject? loadedObj))
                {
                    return CastAndReturn(loadedObj);
                }

                // Remove the weak reference if the target is collected
                s_LoadedObjects.Remove(path);
            }

            AssetImporter importer = GetAssetImporterAtPath(path) ??
                throw new NotSupportedException("Unsupported asset: " + path);

            string fullPath = Path.Combine(Application.DataPath, path);
            EngineObject obj = importer.Import(fullPath);

            s_LoadedObjects.Add(path, new WeakReference<EngineObject>(obj));
            s_ObjectFullPath.Add(obj, fullPath);

            return CastAndReturn(obj);

            static T CastAndReturn(EngineObject loadedObject)
            {
                return (loadedObject is T t) ? t : throw new ArgumentException("Load EngineObject with wrong type");
            }
        }

        internal static string ImporterPathSuffix => ".meta";

        private static string GetImporterPath(string path)
        {
            return path + ImporterPathSuffix;
        }

        public static AssetImporter? GetAssetImporterAtPath(string path)
        {
            AssetImporter? importer = null;

            if (s_LoadedImporters.TryGetValue(path, out WeakReference<AssetImporter>? importerRef))
            {
                if (!importerRef.TryGetTarget(out importer))
                {
                    s_LoadedImporters.Remove(path);
                }
            }

            if (importer == null)
            {
                string importerFullPath = Path.Combine(Application.DataPath, GetImporterPath(path));

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

                    PersistentManager.Save(importer, importerFullPath);
                }

                importer.AssetPath = path.Replace('\\', '/');
                s_LoadedImporters.Add(path, new WeakReference<AssetImporter>(importer));
            }

            return importer;
        }

        public static void MakePersistent(EngineObject obj, string path)
        {
            if (s_LoadedObjects.ContainsKey(path))
            {
                throw new InvalidOperationException($"Object with the same path ({path}) is already loaded");
            }

            s_LoadedObjects.Add(path, new WeakReference<EngineObject>(obj));
            s_ObjectFullPath.AddOrUpdate(obj, Path.Combine(Application.DataPath, path));
        }

        public static bool IsPersistent(EngineObject obj)
        {
            return IsPersistent(obj, out _);
        }

        private static bool IsPersistent(EngineObject obj, [NotNullWhen(true)] out string? fullPath)
        {
            return s_ObjectFullPath.TryGetValue(obj, out fullPath);
        }

        public static void Save(EngineObject obj)
        {
            if (!IsPersistent(obj, out string? fullPath))
            {
                throw new ArgumentException("Can not save non-persistent EngineObject directly");
            }

            PersistentManager.Save(obj, fullPath);
        }

        public static void Save(EngineObject obj, string path)
        {
            string fullPath = Path.Combine(Application.DataPath, path);
            PersistentManager.Save(obj, fullPath);
        }

        public static void MarkDirty(EngineObject obj)
        {
            s_DirtyObjects.Add(obj);
        }

        public static void SaveAllDirtyObjects()
        {
            foreach (var obj in s_DirtyObjects)
            {
                try
                {
                    Save(obj);
                }
                catch (Exception e)
                {
                    Debug.LogError("Failed to save dirty object: " + e.Message);
                }
            }

            s_DirtyObjects.Clear();
        }

        private static bool TryCreateAssetImporter(string path, [NotNullWhen(true)] out AssetImporter? importer)
        {
            if (s_AssetImporterTypeCacheVersion != TypeCache.Version)
            {
                RebuildAssetImporterTypeMap();
            }

            string fullPath = Path.Combine(Application.DataPath, path);

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

        private static void RebuildAssetImporterTypeMap()
        {
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
    }
}
