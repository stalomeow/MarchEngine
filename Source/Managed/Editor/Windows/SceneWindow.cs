using March.Core;
using System.Numerics;

namespace March.Editor.Windows
{
    internal static class SceneWindow
    {
        private static GameObject? s_EditorSceneCamera;

        internal static void InitSceneWindow()
        {
            s_EditorSceneCamera = new GameObject();
            s_EditorSceneCamera.AwakeRecursive();

            s_EditorSceneCamera.transform.Position = new Vector3(0, 5, -5);
            s_EditorSceneCamera.transform.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitX, MathF.PI / 4.0f);

            Camera camera = s_EditorSceneCamera.AddComponent<Camera>();
            camera.IsEditorSceneCamera = true;
        }
    }
}
