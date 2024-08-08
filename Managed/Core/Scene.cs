using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DX12Demo.Core
{
    public class Scene : EngineObject
    {
        [JsonProperty] public string Name = "New Scene";
        [JsonProperty] public List<GameObject> RootGameObjects = [];
    }
}
