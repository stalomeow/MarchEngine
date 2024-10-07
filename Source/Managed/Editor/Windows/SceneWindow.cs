using March.Core;
using March.Core.Rendering;
using System.Numerics;

namespace March.Editor.Windows
{
    internal static class SceneWindow
    {
        private static readonly Scene s_DummyScene = new();
        private static readonly Material s_GridMaterial = new();

        internal static void InitSceneWindow()
        {
            GameObject go = s_DummyScene.CreateGameObject("EditorSceneCamera");

            go.transform.Position = new Vector3(0, 5, -5);
            go.transform.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitX, MathF.PI / 4.0f);

            Camera camera = go.AddComponent<Camera>();
            camera.IsEditorSceneCamera = true;

            Shader gridShader = AssetDatabase.Load<Shader>("Assets/Shaders/Builtin/SceneViewGrid.shader")!;
            s_GridMaterial.Shader = gridShader;
            RenderPipeline.SetSceneViewGridMaterial(s_GridMaterial);
        }
    }
}
