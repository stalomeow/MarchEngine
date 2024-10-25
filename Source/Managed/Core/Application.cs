using March.Core.Binding;
using System.Runtime.InteropServices;

namespace March.Core
{
    public static partial class Application
    {
        private static string? s_CachedDataPath;

        public static string DataPath
        {
            get
            {
                if (s_CachedDataPath == null)
                {
                    nint path = Application_GetDataPath();
                    s_CachedDataPath = NativeString.GetAndFree(path);
                }

                return s_CachedDataPath;
            }
        }

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

            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        #region Bindings

        [NativeFunction]
        private static partial nint Application_GetDataPath();

        #endregion
    }
}
