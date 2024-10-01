using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Reflection;
using System.Text;

namespace March.Core.Serialization
{
    /// <summary>
    /// 禁止使用 <see cref="Guid"/> 进行序列化
    /// </summary>
    public interface IForceInlineSerialization { }

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

        public static MarchObject Load(TextReader textReader)
        {
            return Load<MarchObject>(textReader);
        }

        public static T Load<T>(TextReader textReader) where T : MarchObject
        {
            using var jsonReader = new JsonTextReader(textReader);
            return s_JsonSerializer.Deserialize<T>(jsonReader) ?? throw new LoadPersistentObjectException($"Failed to Load {nameof(MarchObject)}");
        }

        public static MarchObject Load(string fullPath)
        {
            return Load<MarchObject>(fullPath);
        }

        public static T Load<T>(string fullPath) where T : MarchObject
        {
            using var streamReader = new StreamReader(fullPath, Encoding.UTF8);
            return Load<T>(streamReader);
        }

        public static string LoadString(string fullPath)
        {
            return File.ReadAllText(fullPath, Encoding.UTF8);
        }

        public static MarchObject LoadFromString(string text)
        {
            return LoadFromString<MarchObject>(text);
        }

        public static T LoadFromString<T>(string text) where T : MarchObject
        {
            using var stringReader = new StringReader(text);
            return Load<T>(stringReader);
        }

        public static void Overwrite(string fullPath, MarchObject target)
        {
            using var streamReader = new StreamReader(fullPath, Encoding.UTF8);
            Overwrite(streamReader, target);
        }

        public static void Overwrite(TextReader textReader, MarchObject target)
        {
            using var jsonReader = new JsonTextReader(textReader);
            s_JsonSerializer.Populate(jsonReader, target);
        }

        public static void OverwriteFromString(string text, MarchObject target)
        {
            using var stringReader = new StringReader(text);
            Overwrite(stringReader, target);
        }

        public static void Save(MarchObject obj, TextWriter textWriter)
        {
            using var jsonWriter = new JsonTextWriter(textWriter);
            s_JsonSerializer.Serialize(jsonWriter, obj, typeof(MarchObject));
        }

        public static void Save(MarchObject obj, string fullPath)
        {
            string? directory = Path.GetDirectoryName(fullPath);

            if (directory != null && !Directory.Exists(directory))
            {
                Directory.CreateDirectory(directory);
            }

            using var streamWriter = new StreamWriter(fullPath, append: false, Encoding.UTF8);
            Save(obj, streamWriter);
        }

        public static string SaveAsString(MarchObject obj)
        {
            using var stringWriter = new StringWriter();
            Save(obj, stringWriter);
            return stringWriter.ToString();
        }

        private sealed class MarchObjectGuidJsonConverter : JsonConverter<MarchObject>
        {
            public override MarchObject? ReadJson(JsonReader reader, Type objectType, MarchObject? existingValue, bool hasExistingValue, JsonSerializer serializer)
            {
                string? guid = reader.Value?.ToString();
                return guid == null ? null : AssetManager.LoadByGuid(guid);
            }

            public override void WriteJson(JsonWriter writer, MarchObject? value, JsonSerializer serializer)
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

                if (prop.PropertyType != null)
                {
                    if (UseGuidSerialization(prop.PropertyType))
                    {
                        prop.Converter = new MarchObjectGuidJsonConverter();
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

                            if (def == typeof(IDictionary<,>) && UseGuidSerialization(i.GetGenericArguments()[1]))
                            {
                                prop.ItemConverter = new MarchObjectGuidJsonConverter();
                                break;
                            }

                            if (def == typeof(IEnumerable<>) && UseGuidSerialization(i.GetGenericArguments()[0]))
                            {
                                prop.ItemConverter = new MarchObjectGuidJsonConverter();
                                break;
                            }
                        }
                    }
                }

                return prop;
            }

            private static bool UseGuidSerialization(Type type)
            {
                return type.IsSubclassOf(typeof(MarchObject)) && !typeof(IForceInlineSerialization).IsAssignableFrom(type);
            }
        }
    }
}
