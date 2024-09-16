using Newtonsoft.Json;

namespace March.Core
{
    public sealed class SceneAsset : EngineObject
    {
        [JsonProperty]
        internal string? JsonData;
    }
}
