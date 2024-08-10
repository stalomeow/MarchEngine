using System.Diagnostics.CodeAnalysis;

namespace DX12Demo.Editor
{
    internal sealed class DrawerCache<T>(Type genericTypeDefinition) where T : class, IDrawer
    {
        private readonly Dictionary<Type, T> m_Drawers = [];
        private readonly Dictionary<Type, int> m_NotFound = [];

        public bool TryGet(Type type, [NotNullWhen(true)] out T? drawer)
        {
            if (m_Drawers.TryGetValue(type, out drawer))
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
                Type searchType;

                try
                {
                    searchType = genericTypeDefinition.MakeGenericType(t);
                }
                catch (ArgumentException)
                {
                    // Any element of typeArguments does not satisfy the constraints specified for
                    // the corresponding type parameter of the current generic type.
                    goto NextTurn;
                }

                foreach (var drawerType in TypeCache.GetTypesDerivedFrom(searchType))
                {
                    try
                    {
                        drawer = Activator.CreateInstance(drawerType) as T;

                        if (drawer == null)
                        {
                            continue;
                        }

                        m_Drawers.Add(type, drawer);
                        return true;
                    }
                    catch
                    {
                        continue;
                    }
                }

            NextTurn:
                t = t.BaseType;
            }

            m_NotFound.Add(type, TypeCache.Version);
            return false;
        }
    }
}
