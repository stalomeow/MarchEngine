using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Core.Serialization
{
    internal class AssetReferenceJsonConverter : JsonConverter
    {
        public override bool CanConvert(Type objectType)
        {
            return objectType.IsGenericType && objectType.GetGenericTypeDefinition() == typeof(AssetReference<>);
        }

        public override object? ReadJson(JsonReader reader, Type objectType, object? existingValue, JsonSerializer serializer)
        {
            string? guid = reader.Value?.ToString();
            EngineObject? value = guid == null ? null : AssetManager.LoadByGuid(guid);
            return Activator.CreateInstance(objectType, value);
        }

        public override void WriteJson(JsonWriter writer, object? value, JsonSerializer serializer)
        {
            if (value == null)
            {
                writer.WriteNull();
                return;
            }

            var contract = (JsonObjectContract)serializer.ContractResolver.ResolveContract(value.GetType());
            IValueProvider? provider = contract.Properties[nameof(AssetReference<EngineObject>.Value)].ValueProvider;
            EngineObject? asset = provider?.GetValue(value) as EngineObject;

            if (asset?.PersistentGuid == null)
            {
                writer.WriteNull();
            }
            else
            {
                writer.WriteValue(asset.PersistentGuid);
            }
        }
    }
}
