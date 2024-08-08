using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
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
            using var streamReader = new StreamReader(path, Encoding.UTF8);
            using var jsonReader = new JsonTextReader(streamReader);
            return s_JsonSerializer.Deserialize<T>(jsonReader) ?? throw new LoadPersistentObjectException("Failed to Load EngineObject");
        }

        public static void Save(string path, EngineObject obj)
        {
            using var streamWriter = new StreamWriter(path, append: false, Encoding.UTF8);
            using var jsonWriter = new JsonTextWriter(streamWriter);
            s_JsonSerializer.Serialize(jsonWriter, obj, typeof(EngineObject));
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
