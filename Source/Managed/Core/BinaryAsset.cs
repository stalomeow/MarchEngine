using Newtonsoft.Json;

namespace March.Core
{
    public class BinaryAsset : MarchObject
    {
        [JsonProperty]
        public byte[] Data { get; internal set; } = [];
    }
}
