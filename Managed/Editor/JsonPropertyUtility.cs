using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor
{
    public static class JsonPropertyUtility
    {
        public static T? GetValue<T>(this JsonProperty property, object target)
        {
            IValueProvider? valueProvider = property.ValueProvider;

            if (valueProvider == null)
            {
                throw new InvalidOperationException("ValueProvider is null");
            }

            return (T?)valueProvider.GetValue(target);
        }

        public static void SetValue(this JsonProperty property, object target, object? value)
        {
            IValueProvider? valueProvider = property.ValueProvider;

            if (valueProvider == null)
            {
                throw new InvalidOperationException("ValueProvider is null");
            }

            valueProvider.SetValue(target, value);
        }

        public static T? GetAttribute<T>(this JsonProperty property, bool inherit = true) where T : Attribute
        {
            IAttributeProvider? attributeProvider = property.AttributeProvider;

            if (attributeProvider == null)
            {
                return null;
            }

            IList<Attribute> attributes = attributeProvider.GetAttributes(typeof(T), inherit);
            return attributes.Count > 0 ? attributes[0] as T : null;
        }

        public static string GetDisplayName(this JsonProperty property)
        {
            return property.PropertyName ?? property.UnderlyingName ?? "<unknown>";
        }

        public static string GetTooltip(this JsonProperty property)
        {
            var attr = property.GetAttribute<TooltipAttribute>();
            return attr?.Tooltip ?? string.Empty;
        }
    }
}
