using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;
using System.Collections;

namespace DX12Demo.Editor
{
    public enum EditorPropertyKind
    {
        /// <summary>
        /// 对象的成员
        /// </summary>
        Member = 0,

        /// <summary>
        /// <see cref="IList"/> 的元素
        /// </summary>
        ListItem = 1,

        /// <summary>
        /// <see cref="IDictionary"/> 的元素
        /// </summary>
        DictionaryItem = 2,
    }

    public readonly struct EditorProperty
    {
        private readonly object m_Target;
        private readonly int m_ListIndex;
        private readonly object? m_DictionaryKey;

        public JsonProperty Json { get; }

        public EditorPropertyKind Kind { get; }

        private EditorProperty(object target, JsonProperty json, EditorPropertyKind kind, int listIndex, object? dictionaryKey)
        {
            m_Target = target;
            m_ListIndex = listIndex;
            m_DictionaryKey = dictionaryKey;
            Json = json;
            Kind = kind;
        }

        public EditorProperty(object target, JsonProperty json)
            : this(target, json, EditorPropertyKind.Member, -1, null) { }

        public readonly T? GetValue<T>() where T : notnull
        {
            IValueProvider? valueProvider = Json.ValueProvider ?? throw new InvalidOperationException("ValueProvider is null");
            object? value = valueProvider.GetValue(m_Target);

            switch (Kind)
            {
                case EditorPropertyKind.Member:
                    // Do nothing
                    break;

                case EditorPropertyKind.ListItem:
                    if (value is not IList list)
                    {
                        throw new InvalidOperationException("Property is not IList");
                    }

                    value = list[m_ListIndex];
                    break;

                case EditorPropertyKind.DictionaryItem:
                    if (value is not IDictionary dictionary)
                    {
                        throw new InvalidOperationException("Property is not IDictionary");
                    }

                    value = dictionary[m_DictionaryKey!];
                    break;

                default:
                    throw new NotSupportedException($"Unsupported {nameof(EditorPropertyKind)}: {Kind}");
            }

            return (T?)value;
        }

        public readonly void SetValue(object? value)
        {
            IValueProvider? valueProvider = Json.ValueProvider ?? throw new InvalidOperationException("ValueProvider is null");

            switch (Kind)
            {
                case EditorPropertyKind.Member:
                    valueProvider.SetValue(m_Target, value);
                    break;

                case EditorPropertyKind.ListItem:
                    if (valueProvider.GetValue(m_Target) is not IList list)
                    {
                        throw new InvalidOperationException("Property is not IList");
                    }

                    list[m_ListIndex] = value;

                    if (list.GetType().IsValueType)
                    {
                        valueProvider.SetValue(m_Target, list);
                    }
                    break;

                case EditorPropertyKind.DictionaryItem:
                    if (valueProvider.GetValue(m_Target) is not IDictionary dictionary)
                    {
                        throw new InvalidOperationException("Property is not IDictionary");
                    }

                    dictionary[m_DictionaryKey!] = value;

                    if (dictionary.GetType().IsValueType)
                    {
                        valueProvider.SetValue(m_Target, dictionary);
                    }
                    break;

                default:
                    throw new NotSupportedException($"Unsupported {nameof(EditorPropertyKind)}: {Kind}");
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
            get => Kind switch
            {
                EditorPropertyKind.Member => Json.PropertyType,
                EditorPropertyKind.ListItem or EditorPropertyKind.DictionaryItem => GetValue<object>()?.GetType(),
                _ => throw new NotSupportedException("Unsupported EditorPropertyKind"),
            };
        }

        public readonly EditorProperty GetListItemProperty(int index)
        {
            if (Kind != EditorPropertyKind.Member)
            {
                throw new InvalidOperationException();
            }

            return new EditorProperty(m_Target, Json, EditorPropertyKind.ListItem, index, null);
        }

        public readonly EditorProperty GetDictionaryItemProperty(object key)
        {
            if (Kind != EditorPropertyKind.Member)
            {
                throw new InvalidOperationException();
            }

            return new EditorProperty(m_Target, Json, EditorPropertyKind.DictionaryItem, -1, key);
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
    }
}
