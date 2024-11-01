using March.Core.Pool;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Reflection;
using System.Text;

namespace March.Core.Serialization
{
    public class LoadPersistentObjectException(string message) : Exception(message) { }

    public static class PersistentManager
    {
        private static readonly MarchPropertyJsonConverter s_PropertyJsonConverter = new();
        private static readonly ObjectPool<MarchReferenceResolver> s_ReferenceResolverPool = new();
        private static readonly JsonSerializer s_JsonSerializer = JsonSerializer.CreateDefault(new JsonSerializerSettings
        {
            ContractResolver = new ContractResolver(),
            TypeNameHandling = TypeNameHandling.Auto,
            Formatting = Formatting.Indented,
            ConstructorHandling = ConstructorHandling.AllowNonPublicDefaultConstructor,
            ObjectCreationHandling = ObjectCreationHandling.Replace, // 保证不会重新添加集合元素
            PreserveReferencesHandling = PreserveReferencesHandling.Objects,
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
            using (new MarchReferenceScope(s_JsonSerializer, s_ReferenceResolverPool))
            {
                using var jsonReader = new JsonTextReader(textReader);
                return s_JsonSerializer.Deserialize<T>(jsonReader) ?? throw new LoadPersistentObjectException($"Failed to Load {nameof(MarchObject)}");
            }
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
            using (new MarchReferenceScope(s_JsonSerializer, s_ReferenceResolverPool))
            {
                using var jsonReader = new JsonTextReader(textReader);
                s_JsonSerializer.Populate(jsonReader, target);
            }
        }

        public static void OverwriteFromString(string text, MarchObject target)
        {
            using var stringReader = new StringReader(text);
            Overwrite(stringReader, target);
        }

        public static void Save(MarchObject obj, TextWriter textWriter)
        {
            using (new MarchReferenceScope(s_JsonSerializer, s_ReferenceResolverPool))
            {
                using var jsonWriter = new JsonTextWriter(textWriter);
                s_JsonSerializer.Serialize(jsonWriter, obj, typeof(MarchObject));
            }
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

        private sealed class ContractResolver : DefaultContractResolver
        {
            protected override JsonContract CreateContract(Type objectType)
            {
                JsonContract contract = base.CreateContract(objectType);
                contract.IsReference = IsMarchObject(objectType);
                return contract;
            }

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
                    if (IsMarchObject(prop.PropertyType))
                    {
                        prop.IsReference = true;
                        prop.ItemIsReference = false;
                        prop.Converter = s_PropertyJsonConverter;
                    }
                    else if (IsCollectionOfMarchObjects(prop.PropertyType))
                    {
                        prop.IsReference = false;
                        prop.ItemIsReference = true;
                        prop.ItemConverter = s_PropertyJsonConverter;
                    }
                    else
                    {
                        prop.IsReference = false;
                        prop.ItemIsReference = false;
                    }
                }

                return prop;
            }

            private static bool IsMarchObject(Type type)
            {
                return type.IsSubclassOf(typeof(MarchObject));
            }

            private static bool IsCollectionOfMarchObjects(Type type)
            {
                foreach (Type i in type.GetInterfaces())
                {
                    if (!i.IsGenericType)
                    {
                        continue;
                    }

                    Type def = i.GetGenericTypeDefinition();

                    if (def == typeof(IEnumerable<>) || def == typeof(IDictionary<,>))
                    {
                        if (i.GetGenericArguments().Any(IsMarchObject))
                        {
                            return true;
                        }
                    }
                }

                return false;
            }
        }
    }
}
