using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Reflection;
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
            ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor,
            ObjectCreationHandling = ObjectCreationHandling.Replace, // 保证不会重新添加集合元素
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

        public static string LoadString(string fullPath)
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

        public static void Overwrite(string fullPath, EngineObject target)
        {
            using var streamReader = new StreamReader(fullPath, Encoding.UTF8);
            Overwrite(streamReader, target);
        }

        public static void Overwrite(TextReader textReader, EngineObject target)
        {
            using var jsonReader = new JsonTextReader(textReader);
            s_JsonSerializer.Populate(jsonReader, target);
        }

        public static void OverwriteFromString(string text, EngineObject target)
        {
            using var stringReader = new StringReader(text);
            Overwrite(stringReader, target);
        }

        public static void Save(EngineObject obj, TextWriter textWriter)
        {
            using var jsonWriter = new JsonTextWriter(textWriter);
            s_JsonSerializer.Serialize(jsonWriter, obj, typeof(EngineObject));
        }

        public static void Save(EngineObject obj, string fullPath)
        {
            string? directory = Path.GetDirectoryName(fullPath);

            if (directory != null && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }

            using var streamWriter = new StreamWriter(fullPath, append: false, Encoding.UTF8);
            Save(obj, streamWriter);
        }

        public static string SaveAsString(EngineObject obj)
        {
            using var stringWriter = new StringWriter();
            Save(obj, stringWriter);
            return stringWriter.ToString();
        }

        private sealed class EngineObjectGuidJsonConverter : JsonConverter<EngineObject>
        {
            public override EngineObject? ReadJson(JsonReader reader, Type objectType, EngineObject? existingValue, bool hasExistingValue, JsonSerializer serializer)
            {
                string? guid = reader.Value?.ToString();
                return guid == null ? null : AssetManager.LoadByGuid(guid);
            }

            public override void WriteJson(JsonWriter writer, EngineObject? value, JsonSerializer serializer)
            {
                if (value?.PersistentGuid == null)
                {
                    writer.WriteComment("Not Persistent");
                    writer.WriteNull();
                }
                else
                {
                    writer.WriteValue(value.PersistentGuid);
                }
            }
        }

        private sealed class ContractResolver : DefaultContractResolver
        {
            protected override JsonProperty CreateProperty(MemberInfo member, MemberSerialization memberSerialization)
            {
                JsonProperty prop = base.CreateProperty(member, memberSerialization);

                if (!prop.Readable || !prop.Writable || prop.ValueProvider == null)
                {
#pragma warning disable CS8603   // Possible null reference return.
                    return null; // 返回 null 表示忽略该属性
#pragma warning restore CS8603   // Possible null reference return.
                }

                // 让 EngineObject 类型的 Property 使用 Guid 序列化

                if (prop.PropertyType != null)
                {
                    if (prop.PropertyType.IsSubclassOf(typeof(EngineObject)))
                    {
                        prop.Converter = new EngineObjectGuidJsonConverter();
                    }
                    else
                    {
                        foreach (Type i in prop.PropertyType.GetInterfaces())
                        {
                            if (!i.IsGenericType)
                            {
                                continue;
                            }

                            Type def = i.GetGenericTypeDefinition();

                            if (def == typeof(IEnumerable<>) && i.GetGenericArguments()[0].IsSubclassOf(typeof(EngineObject)))
                            {
                                prop.ItemConverter = new EngineObjectGuidJsonConverter();
                                break;
                            }

                            if (def == typeof(IDictionary<,>) && i.GetGenericArguments()[1].IsSubclassOf(typeof(EngineObject)))
                            {
                                prop.ItemConverter = new EngineObjectGuidJsonConverter();
                                break;
                            }
                        }
                    }
                }

                return prop;
            }
        }
    }
}
