using March.Core.Pool;
using System.Reflection;
using System.Runtime.Loader;

namespace March.Editor
{
    public static class TypeCache
    {
        private record class EnumData(string[] Names, Array Values, Enum[] BoxedValues);

        private static readonly Dictionary<Type, HashSet<Type>> s_Types = [];
        private static readonly Dictionary<Type, EnumData> s_EnumCache = [];

        public static int Version { get; private set; }

        static TypeCache()
        {
            foreach (Assembly assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                AddTypes(assembly);
            }

            AppDomain.CurrentDomain.AssemblyLoad += OnLoadAssembly;
        }

        public static void RemoveTypesIn(Assembly assembly)
        {
            Type[] types = assembly.GetTypes();

            foreach (Type type in types)
            {
                s_Types.Remove(type);
                s_EnumCache.Remove(type);
            }

            using var typesToRemove = HashSetPool<Type>.Get();

            foreach (KeyValuePair<Type, HashSet<Type>> kv in s_Types)
            {
                kv.Value.ExceptWith(types);

                if (kv.Value.Count == 0)
                {
                    typesToRemove.Value.Add(kv.Key);
                }
            }

            foreach (Type type in typesToRemove.Value)
            {
                s_Types.Remove(type);
            }

            Version++;
        }

        public static void RemoveTypesIn(AssemblyLoadContext context)
        {
            foreach (Assembly assembly in context.Assemblies)
            {
                RemoveTypesIn(assembly);
            }
        }

        public static IEnumerable<Type> GetTypesDerivedFrom(Type type)
        {
            if (s_Types.TryGetValue(type, out HashSet<Type>? types))
            {
                return types;
            }

            return Enumerable.Empty<Type>();
        }

        public static IEnumerable<Type> GetTypesDerivedFrom<T>()
        {
            return GetTypesDerivedFrom(typeof(T));
        }

        public static void GetEnumData(Type type, out ReadOnlySpan<string> names, out ReadOnlySpan<Enum> values)
        {
            EnumData data = GetEnumData(type);
            names = data.Names;
            values = data.BoxedValues;
        }

        public static void GetEnumData<T>(out ReadOnlySpan<string> names, out ReadOnlySpan<T> values) where T : struct, Enum
        {
            EnumData data = GetEnumData(typeof(T));
            names = data.Names;
            values = (T[])data.Values;
        }

        private static EnumData GetEnumData(Type type)
        {
            if (!s_EnumCache.TryGetValue(type, out EnumData? data))
            {
                string[] names = type.GetEnumNames();
                Array values = type.GetEnumValues();
                Enum[] boxedValues = new Enum[values.Length];

                for (int i = 0; i < values.Length; i++)
                {
                    boxedValues[i] = (Enum)values.GetValue(i)!;
                }

                data = new EnumData(names, values, boxedValues);
                s_EnumCache.Add(type, data);
            }

            return data;
        }

        private static void OnLoadAssembly(object? sender, AssemblyLoadEventArgs args)
        {
            AddTypes(args.LoadedAssembly);
        }

        private static void AddTypes(Assembly assembly)
        {
            foreach (Type type in assembly.GetTypes())
            {
                Type? baseType = type.BaseType;
                while (baseType != null && baseType != typeof(object))
                {
                    GetTypeSet(baseType).Add(type);
                    baseType = baseType.BaseType;
                }

                foreach (Type i in type.GetInterfaces())
                {
                    GetTypeSet(i).Add(type);
                }
            }

            Version++;
        }

        private static HashSet<Type> GetTypeSet(Type type)
        {
            if (!s_Types.TryGetValue(type, out HashSet<Type>? types))
            {
                types = new HashSet<Type>();
                s_Types.Add(type, types);
            }

            return types;
        }
    }
}
