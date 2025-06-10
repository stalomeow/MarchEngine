using Newtonsoft.Json;

namespace March.Core.Serialization
{
    internal sealed class MarchPropertyJsonConverter : JsonConverter<MarchObject>
    {
        // 这个对象会被复用，所以不能有任何状态

        public override MarchObject? ReadJson(JsonReader reader, Type objectType, MarchObject? existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            return reader.TokenType switch
            {
                JsonToken.Null => null,
                JsonToken.StartObject => serializer.Deserialize<MarchObject>(reader),
                JsonToken.String => AssetManager.LoadByGuid((string)reader.Value!),
                _ => throw new NotSupportedException("unexpected json token type: " + reader.TokenType)
            };
        }

        public override void WriteJson(JsonWriter writer, MarchObject? value, JsonSerializer serializer)
        {
            if (value == null)
            {
                writer.WriteNull();
            }
            else if (value.PersistentGuid == null)
            {
                serializer.Serialize(writer, value, typeof(MarchObject));
            }
            else
            {
                writer.WriteValue(value.PersistentGuid);
            }
        }
    }
}
