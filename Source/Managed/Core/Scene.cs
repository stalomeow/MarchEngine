using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    public class Scene : EngineObject, IForceInlineSerialization
    {
        [JsonProperty] public string Name = "New Scene";
        [JsonProperty] public List<GameObject> RootGameObjects = [];
    }
}
