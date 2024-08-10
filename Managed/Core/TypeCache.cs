using System.Reflection;
using System.Runtime.Loader;

namespace DX12Demo.Core
{
    public static class TypeCache
    {
        private static readonly Dictionary<Type, HashSet<Type>> s_Types = [];

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
            }

            foreach (HashSet<Type> typeSet in s_Types.Values)
            {
                typeSet.ExceptWith(types);
            }
        }

        public static void RemoveTypesIn(AssemblyLoadContext context)
        {
            foreach (Assembly assembly in context.Assemblies)
            {
                RemoveTypesIn(assembly);
            }
        }

        public static IEnumerable<Type> GetTypeDerivedFrom(Type type)
        {
            if (s_Types.TryGetValue(type, out HashSet<Type>? types))
            {
                return types;
            }

            return Enumerable.Empty<Type>();
        }

        public static IEnumerable<Type> GetTypeDerivedFrom<T>()
        {
            return GetTypeDerivedFrom(typeof(T));
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
