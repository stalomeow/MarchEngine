using Newtonsoft.Json;

namespace March.Core
{
    public sealed class SceneAsset : MarchObject
    {
        [JsonProperty]
        internal string? JsonData;
    }
}
