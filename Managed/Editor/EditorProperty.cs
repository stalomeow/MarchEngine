using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;
using System.Collections;
using System.Collections.Immutable;

namespace DX12Demo.Editor
{
    internal enum EditorPropertyPathKind
    {
        Member = 0,
        ListItem = 1,
        DictionaryItem = 2,
    }

    internal readonly struct EditorPropertyPath
    {
        public EditorPropertyPathKind Kind { get; }

        private readonly JsonProperty? m_Member;
        private readonly int m_ListIndex;
        private readonly object? m_DictionaryKey;

        private EditorPropertyPath(EditorPropertyPathKind kind, JsonProperty? member, int listIndex, object? dictionaryKey)
        {
            Kind = kind;
            m_Member = member;
            m_ListIndex = listIndex;
            m_DictionaryKey = dictionaryKey;
        }

        public object? GetValue(object? target)
        {
            if (target == null)
            {
                return null;
            }

            switch (Kind)
            {
                case EditorPropertyPathKind.Member when m_Member?.ValueProvider != null:
                    return m_Member.ValueProvider.GetValue(target);

                case EditorPropertyPathKind.ListItem when target is IList list:
                    return list[m_ListIndex];

                case EditorPropertyPathKind.DictionaryItem when target is IDictionary dictionary:
                    return dictionary[m_DictionaryKey!];

                default:
                    throw new NotSupportedException("Unsupported EditorPropertyPathKind or Target");
            }
        }

        public void SetValue(object? target, object? value)
        {
            if (target == null)
            {
                return;
            }

            switch (Kind)
            {
                case EditorPropertyPathKind.Member when m_Member?.ValueProvider != null:
                    m_Member.ValueProvider.SetValue(target, value);
                    break;

                case EditorPropertyPathKind.ListItem when target is IList list:
                    list[m_ListIndex] = value;
                    break;

                case EditorPropertyPathKind.DictionaryItem when target is IDictionary dictionary:
                    dictionary[m_DictionaryKey!] = value;
                    break;

                default:
                    throw new NotSupportedException("Unsupported EditorPropertyPathKind or Target");
            }
        }

        public Type? GetType(Type? targetType)
        {
            switch (Kind)
            {
                case EditorPropertyPathKind.Member when m_Member != null:
                    return m_Member.PropertyType;

                case EditorPropertyPathKind.ListItem:
                    return EditorPropertyUtility.GetListItemType(targetType);

                case EditorPropertyPathKind.DictionaryItem:
                    return EditorPropertyUtility.GetDictionaryItemType(targetType);

                default:
                    throw new NotSupportedException("Unsupported EditorPropertyPathKind or Target");
            }
        }

        public static EditorPropertyPath Member(JsonProperty member)
        {
            return new EditorPropertyPath(EditorPropertyPathKind.Member, member, -1, null);
        }

        public static EditorPropertyPath ListItem(int index)
        {
            return new EditorPropertyPath(EditorPropertyPathKind.ListItem, null, index, null);
        }

        public static EditorPropertyPath DictionaryItem(object key)
        {
            return new EditorPropertyPath(EditorPropertyPathKind.DictionaryItem, null, -1, key);
        }
    }

    public readonly struct EditorProperty
    {
        private readonly object m_Target;
        private readonly ImmutableArray<EditorPropertyPath> m_Paths;

        public JsonProperty Json { get; }

        private EditorProperty(object target, ImmutableArray<EditorPropertyPath> paths, JsonProperty json)
        {
            m_Target = target;
            m_Paths = paths;
            Json = json;
        }

        public EditorProperty(object target, JsonProperty json)
            : this(target, [EditorPropertyPath.Member(json)], json) { }

        public readonly T? GetValue<T>() where T : notnull
        {
            object? value = m_Target;

            foreach (EditorPropertyPath path in m_Paths)
            {
                value = path.GetValue(value);
            }

            return (T?)value;
        }

        public readonly void SetValue(object? value)
        {
            SetValueImpl(m_Target, value, 0);
        }

        private void SetValueImpl(object? target, object? value, int pathIndex)
        {
            if (pathIndex >= m_Paths.Length)
            {
                return;
            }

            EditorPropertyPath path = m_Paths[pathIndex];

            if (pathIndex == m_Paths.Length - 1)
            {
                path.SetValue(target, value);
            }
            else
            {
                object? nextTarget = path.GetValue(target);
                SetValueImpl(nextTarget, value, pathIndex + 1);

                if (nextTarget != null && nextTarget.GetType().IsValueType)
                {
                    path.SetValue(target, nextTarget);
                }
            }
        }

        public readonly T? GetAttribute<T>(bool inherit = true) where T : Attribute
        {
            IAttributeProvider? attributeProvider = Json.AttributeProvider;

            if (attributeProvider == null)
            {
                return null;
            }

            IList<Attribute> attributes = attributeProvider.GetAttributes(typeof(T), inherit);
            return attributes.Count > 0 ? attributes[0] as T : null;
        }

        public readonly string DisplayName
        {
            get => Json.PropertyName ?? Json.UnderlyingName ?? "<unknown>";
        }

        public readonly string Tooltip
        {
            get
            {
                var attr = GetAttribute<TooltipAttribute>();
                return attr?.Tooltip ?? string.Empty;
            }
        }

        public readonly bool IsHidden
        {
            get => Json.Ignored || GetAttribute<HideInInspectorAttribute>() != null;
        }

        public readonly Type? PropertyType
        {
            get
            {
                Type? type = m_Target.GetType();

                foreach (EditorPropertyPath path in m_Paths)
                {
                    type = path.GetType(type);
                }

                return type;
            }
        }

        public readonly EditorProperty GetListItemProperty(int index)
        {
            var newPaths = m_Paths.Add(EditorPropertyPath.ListItem(index));
            return new EditorProperty(m_Target, newPaths, Json);
        }

        public readonly EditorProperty GetDictionaryItemProperty(object key)
        {
            var newPaths = m_Paths.Add(EditorPropertyPath.DictionaryItem(key));
            return new EditorProperty(m_Target, newPaths, Json);
        }
    }

    public static class EditorPropertyUtility
    {
        public static EditorProperty GetEditorProperty(this JsonObjectContract contract, object target, string key)
        {
            return new EditorProperty(target, contract.Properties[key]);
        }

        public static EditorProperty GetEditorProperty(this JsonObjectContract contract, object target, int index)
        {
            return new EditorProperty(target, contract.Properties[index]);
        }

        public static Type? GetListItemType(Type? listType)
        {
            if (listType != null)
            {
                foreach (var i in listType.GetInterfaces())
                {
                    if (i.IsGenericType && i.GetGenericTypeDefinition() == typeof(IList<>))
                    {
                        return i.GetGenericArguments()[0];
                    }
                }

                if (typeof(IList).IsAssignableFrom(listType))
                {
                    return typeof(object);
                }
            }

            return null;
        }

        public static Type? GetDictionaryItemType(Type? dictType)
        {
            if (dictType != null)
            {
                foreach (var i in dictType.GetInterfaces())
                {
                    if (i.IsGenericType && i.GetGenericTypeDefinition() == typeof(IDictionary<,>))
                    {
                        return i.GetGenericArguments()[1];
                    }
                }

                if (typeof(IDictionary).IsAssignableFrom(dictType))
                {
                    return typeof(object);
                }
            }

            return null;
        }
    }
}
