using Newtonsoft.Json;

namespace DX12Demo.Core
{
    public class SceneAsset : EngineObject
    {
        [JsonProperty]
        internal string? JsonData;
    }
}
