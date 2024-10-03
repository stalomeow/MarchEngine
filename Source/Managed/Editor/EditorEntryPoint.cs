using March.Editor.Windows;
using System.Runtime.InteropServices;

namespace March.Editor
{
    internal static class EditorEntryPoint
    {
        [UnmanagedCallersOnly]
        private static void OnNativeInitialize()
        {
            AssetDatabase.InitAssetDatabase();
            SceneWindow.InitSceneWindow();
        }
    }
}
