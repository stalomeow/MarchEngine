using Newtonsoft.Json;

namespace DX12Demo.Core
{
    public class Scene : EngineObject
    {
        [JsonProperty] public string Name = "New Scene";
        [JsonProperty] public List<GameObject> RootGameObjects = [];
    }
}
