using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Text;

namespace DX12Demo.Core
{
    public class LoadPersistentObjectException(string message) : Exception(message) { }

    public static class PersistentManager
    {
        private static readonly JsonSerializer s_JsonSerializer = JsonSerializer.CreateDefault(new JsonSerializerSettings
        {
            ContractResolver = new ContractResolver(),
            TypeNameHandling = TypeNameHandling.Auto,
            Formatting = Formatting.Indented,
        });

        private static readonly Dictionary<string, WeakReference<EngineObject>> s_LoadedObjects = new();
        private static readonly ConditionalWeakTable<EngineObject, string> s_ObjectFullPath = new();
        private static readonly HashSet<EngineObject> s_DirtyObjects = new();

        public static JsonContract ResolveJsonContract(Type type)
        {
            return s_JsonSerializer.ContractResolver.ResolveContract(type);
        }

        public static EngineObject Load(string path)
        {
            return Load<EngineObject>(path);
        }

        public static T Load<T>(string path) where T : EngineObject
        {
            if (s_LoadedObjects.TryGetValue(path, out WeakReference<EngineObject>? weakRef))
            {
                if (weakRef.TryGetTarget(out EngineObject? loadedObj))
                {
                    return (loadedObj is T t) ? t : throw new ArgumentException("Load EngineObject with wrong type");
                }

                // Remove the weak reference if the target is collected
                s_LoadedObjects.Remove(path);
            }

            string fullPath = Path.Combine(Application.DataPath, path);
            using var streamReader = new StreamReader(fullPath, Encoding.UTF8);
            using var jsonReader = new JsonTextReader(streamReader);

            T obj = s_JsonSerializer.Deserialize<T>(jsonReader) ?? throw new LoadPersistentObjectException("Failed to Load EngineObject");
            s_LoadedObjects.Add(path, new WeakReference<EngineObject>(obj));
            s_ObjectFullPath.Add(obj, fullPath);
            return obj;
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

            SaveImpl(obj, fullPath);
        }

        public static void Save(EngineObject obj, string path)
        {
            SaveImpl(obj, Path.Combine(Application.DataPath, path));
        }

        private static void SaveImpl(EngineObject obj, string fullPath)
        {
            using var streamWriter = new StreamWriter(fullPath, append: false, Encoding.UTF8);
            using var jsonWriter = new JsonTextWriter(streamWriter);
            s_JsonSerializer.Serialize(jsonWriter, obj, typeof(EngineObject));
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

        private class ContractResolver : DefaultContractResolver
        {
            protected override IList<JsonProperty> CreateProperties(Type type, MemberSerialization memberSerialization)
            {
                IList<JsonProperty> properties = base.CreateProperties(type, memberSerialization);

                for (int i = properties.Count - 1; i >= 0; i--)
                {
                    JsonProperty prop = properties[i];

                    if (!prop.Readable || !prop.Writable || prop.ValueProvider == null)
                    {
                        properties.RemoveAt(i);
                    }
                }

                return properties;
            }
        }
    }
}
