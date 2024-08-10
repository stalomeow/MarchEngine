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

        public static string GetDisplayName(this JsonProperty property)
        {
            return property.PropertyName ?? property.UnderlyingName ?? "<unknown>";
        }
    }
}
