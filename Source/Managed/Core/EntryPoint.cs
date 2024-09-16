using System.Runtime.InteropServices;

namespace March.Core
{
    internal unsafe partial class EntryPoint
    {
        public static event Action? OnTick;

        [UnmanagedCallersOnly]
        private static void OnNativeInitialize()
        {
            try
            {
                //GameObject light = new();
                //Light l = light.AddComponent<Light>();
                //l.Color = Color.Red;

                //GameObject sphere = new() { Scale = new Vector3(3f) };
                //MeshRenderer mr = sphere.AddComponent<MeshRenderer>();
                //mr.MeshType = MeshType.Sphere;

                //s_Scene.RootGameObjects.Add(light);
                //s_Scene.RootGameObjects.Add(sphere);

                //string scenePath = Path.Combine(Application.DataPath, @"Assets/TestScene.scene");
                //SceneManager.CurrentScene = PersistentManager.Load<Scene>(scenePath);
            }
            catch (Exception e)
            {
                Debug.LogException(e);
            }
        }

        [UnmanagedCallersOnly]
        private static void OnNativeTick()
        {
            try
            {
                OnTick?.Invoke();

                foreach (var gameObject in SceneManager.CurrentScene.RootGameObjects)
                {
                    gameObject.Update();
                }
            }
            catch (Exception e)
            {
                Debug.LogException(e);
            }
        }
    }
}
