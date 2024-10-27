using Newtonsoft.Json.Serialization;
using System.Globalization;

namespace March.Core.Serialization
{
    internal readonly ref struct MarchReferenceScope(MarchReferenceResolver resolver)
    {
        public void Dispose()
        {
            resolver.ClearReferences();
        }
    }

    internal sealed class MarchReferenceResolver : IReferenceResolver
    {
        private readonly Dictionary<string, object> m_KeyToValues = [];
        private readonly Dictionary<object, string> m_ValueToKeys = [];

        public void ClearReferences()
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
