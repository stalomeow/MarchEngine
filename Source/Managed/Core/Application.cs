using March.Core.Interop;
using March.Core.Rendering;
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
            Mesh.DestroyGeometries();

            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        public static string SaveFilePanelInProject(string title, string defaultName, string extension, string path = "Assets")
        {
            using NativeString t = title;
            using NativeString d = defaultName;
            using NativeString e = extension;
            using NativeString p = path;

            nint result = Application_SaveFilePanelInProject(t.Data, d.Data, e.Data, p.Data);
            return NativeString.GetAndFree(result);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint Application_GetDataPath();

        [NativeFunction]
        private static partial nint Application_SaveFilePanelInProject(nint title, nint defaultName, nint extension, nint path);

        #endregion
    }
}
