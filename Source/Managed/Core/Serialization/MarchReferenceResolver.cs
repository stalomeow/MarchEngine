using March.Core.Pool;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Globalization;

namespace March.Core.Serialization
{
    internal readonly ref struct MarchReferenceScope
    {
        private readonly JsonSerializer m_Serializer;
        private readonly IReferenceResolver? m_PrevResolver;
        private readonly PooledObject<MarchReferenceResolver> m_TempResolver;

        public MarchReferenceScope(JsonSerializer serializer, ObjectPool<MarchReferenceResolver> resolverPool)
        {
            m_Serializer = serializer;
            m_PrevResolver = serializer.ReferenceResolver;
            m_TempResolver = resolverPool.Use();
            m_Serializer.ReferenceResolver = m_TempResolver.Value;
        }

        public void Dispose()
        {
            m_Serializer.ReferenceResolver = m_PrevResolver;
            m_TempResolver.Dispose();
        }
    }

    internal sealed class MarchReferenceResolver : IReferenceResolver, IPoolableObject
    {
        private readonly Dictionary<string, object> m_KeyToValues = [];
        private readonly Dictionary<object, string> m_ValueToKeys = [];

        public void Reset()
        {
            m_KeyToValues.Clear();
            m_ValueToKeys.Clear();
        }

        public object ResolveReference(object context, string reference)
        {
            return m_KeyToValues[reference];
        }

        public string GetReference(object context, object value)
        {
            if (!m_ValueToKeys.TryGetValue(value, out string? key))
            {
                key = m_KeyToValues.Count.ToString(CultureInfo.InvariantCulture);
                AddReference(context, key, value);
            }

            return key;
        }

        public void AddReference(object context, string reference, object value)
        {
            m_KeyToValues[reference] = value;
            m_ValueToKeys[value] = reference;
        }

        public bool IsReferenced(object context, object value)
        {
            return m_ValueToKeys.ContainsKey(value);
        }
    }
}
