using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    public class Scene : MarchObject, IForceInlineSerialization
    {
        [JsonProperty] public string Name = "New Scene";
        [JsonProperty] public List<GameObject> RootGameObjects = [];

        public GameObject CreateGameObject(string? name = null, GameObject? parent = null)
        {
            GameObject go = new();
            go.AwakeRecursive();

            if (name != null)
            {
                go.Name = name;
            }

            if (parent == null)
            {
                RootGameObjects.Add(go);
            }
            else
            {
                parent.transform.AddChild(go.transform);
            }

            return go;
        }

        public GameObject Find(string path)
        {
            return null!;
        }
    }
}
