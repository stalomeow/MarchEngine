using March.Core.Rendering;

namespace March.Core
{
    public static class SceneManager
    {
        public static Scene CurrentScene { get; set; } = new();

        internal static void Initialize()
        {
            foreach (var gameObject in CurrentScene.RootGameObjects)
            {
                gameObject.AwakeRecursive();
            }

            Application.OnTick += () => CurrentScene.Update();
            Application.OnQuit += () => CurrentScene.Dispose();
        }

        internal static void InitializeEditor(Func<GameObject, bool> selected)
        {
            Application.OnTick += () =>
            {
                Gizmos.Clear();
                CurrentScene.DrawGizmos(selected);
            };
        }
    }
}
