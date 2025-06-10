using System.Diagnostics.CodeAnalysis;

namespace March.Editor
{
    internal sealed class DrawerCache<T>(Type genericTypeDefinition) where T : class, IDrawer
    {
        private readonly Dictionary<Type, T> m_DrawerInstances = [];
        private readonly Dictionary<Type, Type> m_DrawerTypes = [];
        private readonly Dictionary<Type, int> m_NotFound = [];

        public bool TryGetSharedInstance(Type type, [NotNullWhen(true)] out T? drawer)
        {
            if (m_DrawerInstances.TryGetValue(type, out drawer))
            {
                return true;
            }

            if (TryGetType(type, out Type? drawerType))
            {
                drawer = (T)Activator.CreateInstance(drawerType)!;
                m_DrawerInstances.Add(type, drawer);
                return true;
            }

            return false;
        }

        public bool TryGetType(Type type, [NotNullWhen(true)] out Type? drawerType)
        {
            if (m_DrawerTypes.TryGetValue(type, out drawerType))
            {
                return true;
            }

            if (m_NotFound.TryGetValue(type, out int typeCacheVersion))
            {
                if (typeCacheVersion == TypeCache.Version)
                {
                    return false;
                }

                // TypeCache 变了，需要重新查找一次
                m_NotFound.Remove(type);
            }

            Type? t = type;

            while (t != null)
            {
                if (TryGetDerivedType(genericTypeDefinition, t, out drawerType))
                {
                    m_DrawerTypes.Add(type, drawerType);
                    return true;
                }

                t = t.BaseType;
            }

            foreach (Type i in type.GetInterfaces())
            {
                if (TryGetDerivedType(genericTypeDefinition, i, out drawerType))
                {
                    m_DrawerTypes.Add(type, drawerType);
                    return true;
                }
            }

            m_NotFound.Add(type, TypeCache.Version);
            return false;
        }

        private static bool TryGetDerivedType(Type genericTypeDefinition, Type typeArgument, [NotNullWhen(true)] out Type? type)
        {
            Type searchType;

            try
            {
                searchType = genericTypeDefinition.MakeGenericType(typeArgument);
            }
            catch (ArgumentException)
            {
                // 类型参数不满足约束条件
                type = null;
                return false;
            }

            foreach (var derivedType in TypeCache.GetTypesDerivedFrom(searchType))
            {
                if (derivedType.IsAbstract || derivedType.IsGenericTypeDefinition || derivedType.IsValueType)
                {
                    continue;
                }

                type = derivedType;
                return true;
            }

            type = null;
            return false;
        }
    }
}
