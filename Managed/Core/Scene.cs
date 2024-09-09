using DX12Demo.Core.Serialization;
using Newtonsoft.Json;

namespace DX12Demo.Core
{
    public class Scene : EngineObject, IForceInlineSerialization
    {
        [JsonProperty] public string Name = "New Scene";
        [JsonProperty] public List<GameObject> RootGameObjects = [];
    }
}
