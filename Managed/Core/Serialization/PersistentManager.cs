using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Text;

namespace DX12Demo.Core.Serialization
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

        public static EngineObject Load(TextReader textReader)
        {
            return Load<EngineObject>(textReader);
        }

        public static T Load<T>(TextReader textReader) where T : EngineObject
        {
            using var jsonReader = new JsonTextReader(textReader);
            return s_JsonSerializer.Deserialize<T>(jsonReader) ?? throw new LoadPersistentObjectException("Failed to Load EngineObject");
        }

        public static EngineObject Load(string fullPath)
        {
            return Load<EngineObject>(fullPath);
        }

        public static T Load<T>(string fullPath) where T : EngineObject
        {
            using var streamReader = new StreamReader(fullPath, Encoding.UTF8);
            return Load<T>(streamReader);
        }

        public static string LoadAsString(string fullPath)
        {
            return File.ReadAllText(fullPath, Encoding.UTF8);
        }

        public static EngineObject LoadFromString(string text)
        {
            return LoadFromString<EngineObject>(text);
        }

        public static T LoadFromString<T>(string text) where T : EngineObject
        {
            using var stringReader = new StringReader(text);
            return Load<T>(stringReader);
        }

        public static void Save(EngineObject obj, TextWriter textWriter)
        {
            using var jsonWriter = new JsonTextWriter(textWriter);
            s_JsonSerializer.Serialize(jsonWriter, obj, typeof(EngineObject));
        }

        public static void Save(EngineObject obj, string fullPath)
        {
            using var streamWriter = new StreamWriter(fullPath, append: false, Encoding.UTF8);
            Save(obj, streamWriter);
        }

        public static string SaveAsString(EngineObject obj)
        {
            using var stringWriter = new StringWriter();
            Save(obj, stringWriter);
            return stringWriter.ToString();
        }

        private sealed class ContractResolver : DefaultContractResolver
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
