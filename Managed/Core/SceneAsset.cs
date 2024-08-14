using Newtonsoft.Json;

namespace DX12Demo.Core
{
    public sealed class SceneAsset : EngineObject
    {
        [JsonProperty]
        internal string? JsonData;
    }
}
