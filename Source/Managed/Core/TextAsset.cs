using Newtonsoft.Json;

namespace March.Core
{
    public class TextAsset : MarchObject
    {
        [JsonProperty]
        public string Text { get; internal set; } = string.Empty;
    }
}
