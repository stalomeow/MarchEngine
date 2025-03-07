using March.Core.Rendering;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    public class Scene : MarchObject, IDisposable
    {
        [JsonProperty]
        [HideInInspector]
        public string Name { get; set; } = "New Scene";

        [JsonProperty]
        [HideInInspector]
        public List<GameObject> RootGameObjects { get; private set; } = new();

        [JsonProperty]
        [HideInInspector]
        public Material? SkyboxMaterial { get; set; } = null;

        [JsonProperty]
        [HideInInspector]
        public Texture? EnvironmentRadianceMap { get; set; } = null;

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

        public void AddRootGameObject(GameObject go)
        {
            go.AwakeRecursive();
            RootGameObjects.Add(go);
        }

        public GameObject? Find(string path)
        {
            GameObject? current = null;

            foreach (string name in path.Split('/', StringSplitOptions.None))
            {
                bool success = false;

                if (current == null)
                {
                    current = RootGameObjects.Find(go => go.Name == name);
                    success = current != null;
                }
                else
                {
                    for (int i = 0; i < current.transform.ChildCount; i++)
                    {
                        GameObject child = current.transform.GetChild(i).gameObject;

                        if (child.Name == name)
                        {
                            current = child;
                            success = true;
                            break;
                        }
                    }
                }

                if (!success)
                {
                    return null;
                }
            }

            return current;
        }

        public void Update()
        {
            foreach (GameObject go in RootGameObjects)
            {
                go.UpdateRecursive();
            }
        }

        public void DrawGizmos(Func<GameObject, bool> selected)
        {
            foreach (GameObject go in RootGameObjects)
            {
                go.DrawGizmosRecursive(selected);
            }
        }

        public void DrawGizmosGUI(Func<GameObject, bool> selected)
        {
            foreach (GameObject go in RootGameObjects)
            {
                go.DrawGizmosGUIRecursive(selected);
            }
        }

        public void Dispose()
        {
            foreach (GameObject go in RootGameObjects)
            {
                go.Dispose();
            }

            RootGameObjects.Clear();

            SkyboxMaterial = null;
            EnvironmentRadianceMap = null;
        }
    }
}
