using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    public class Scene : MarchObject, IForceInlineSerialization
    {
        [JsonProperty] public string Name = "New Scene";
        [JsonProperty] public List<GameObject> RootGameObjects = [];

        public void AddGameObject(GameObject? parent, GameObject go)
        {
            go.AwakeRecursive();

            if (parent == null)
            {
                RootGameObjects.Add(go);
            }
            else
            {
                parent.transform.AddChild(go.transform);
            }
        }
    }
}
