using March.Core.Interop;
using March.Core.Rendering;
using System.Runtime.InteropServices;

namespace March.Core
{
    public static partial class Application
    {
        [UnmanagedCallersOnly]
        private static void OnStart()
        {
            foreach (var gameObject in SceneManager.CurrentScene.RootGameObjects)
            {
                gameObject.AwakeRecursive();
            }
        }

        [UnmanagedCallersOnly]
        private static void OnTick()
        {
            SceneManager.CurrentScene.Update();
        }

        [UnmanagedCallersOnly]
        private static void OnQuit()
        {
            SceneManager.CurrentScene.Dispose();
            Mesh.DestroyGeometries();
            Texture.DestroyDefaults();

            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        private static string? s_CachedDataPath;

        public static string DataPath => s_CachedDataPath ??= GetDataPath();

        [NativeMethod]
        private static partial string GetDataPath();

        [NativeMethod]
        public static partial string SaveFilePanelInProject(string title, string defaultName, string extension, string path = "Assets");
    }
}
