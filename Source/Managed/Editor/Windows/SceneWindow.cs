using March.Core;
using System.Numerics;

namespace March.Editor.Windows
{
    internal static class SceneWindow
    {
        private static readonly Scene s_DummyScene = new();

        internal static void InitSceneWindow()
        {
            GameObject go = s_DummyScene.CreateGameObject("EditorSceneCamera");

            go.transform.Position = new Vector3(0, 5, -5);
            go.transform.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitX, MathF.PI / 4.0f);

            Camera camera = go.AddComponent<Camera>();
            camera.IsEditorSceneCamera = true;
        }
    }
}
