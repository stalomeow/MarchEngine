using March.Core;
using March.Core.Binding;
using March.Core.IconFonts;
using March.Core.Rendering;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Scene")]
    internal unsafe partial class SceneWindow : EditorWindow
    {
        private readonly Material m_GridMaterial = new();
        private readonly Scene m_DummyScene = new();
        private readonly Camera m_SceneViewCamera;

        public SceneWindow() : base(SceneWindow_New(), $"{FontAwesome6.VectorSquare} Scene")
        {
            DefaultSize = new Vector2(960.0f, 540.0f);

            GameObject go = m_DummyScene.CreateGameObject("EditorSceneCamera");
            go.transform.Position = new Vector3(0, 5, -5);
            go.transform.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitX, MathF.PI / 4.0f);

            m_SceneViewCamera = go.AddComponent<Camera>();
            m_SceneViewCamera.EnableGizmos = true;

            m_GridMaterial.Shader = AssetDatabase.Load<Shader>("Engine/Shaders/SceneViewGrid.shader")!;
        }

        protected override void OnDispose(bool disposing)
        {
            m_GridMaterial.Dispose();
            m_DummyScene.Dispose();

            SceneWindow_Delete(NativePtr);
            base.OnDispose(disposing);
        }

        [JsonProperty]
        public Vector3 CameraPosition
        {
            get => m_SceneViewCamera.gameObject.transform.Position;
            set => m_SceneViewCamera.gameObject.transform.Position = value;
        }

        [JsonProperty]
        public Quaternion CameraRotation
        {
            get => m_SceneViewCamera.gameObject.transform.Rotation;
            set => m_SceneViewCamera.gameObject.transform.Rotation = value;
        }

        [JsonProperty]
        public float MouseSensitivity
        {
            get => SceneWindow_GetMouseSensitivity(NativePtr);
            set => SceneWindow_SetMouseSensitivity(NativePtr, value);
        }

        [JsonProperty]
        public float RotateDegSpeed
        {
            get => SceneWindow_GetRotateDegSpeed(NativePtr);
            set => SceneWindow_SetRotateDegSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float NormalMoveSpeed
        {
            get => SceneWindow_GetNormalMoveSpeed(NativePtr);
            set => SceneWindow_SetNormalMoveSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float FastMoveSpeed
        {
            get => SceneWindow_GetFastMoveSpeed(NativePtr);
            set => SceneWindow_SetFastMoveSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float PanSpeed
        {
            get => SceneWindow_GetPanSpeed(NativePtr);
            set => SceneWindow_SetPanSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float ZoomSpeed
        {
            get => SceneWindow_GetZoomSpeed(NativePtr);
            set => SceneWindow_SetZoomSpeed(NativePtr, value);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            DrawMenuBar();
            RenderCamera();
            SceneWindow_DrawSceneView(NativePtr);
            TravelScene();
        }

        private void DrawMenuBar()
        {
            bool enableWireframe = m_SceneViewCamera.EnableWireframe;
            SceneWindow_DrawMenuBar(NativePtr, &enableWireframe);
            m_SceneViewCamera.EnableWireframe = enableWireframe;
        }

        private void RenderCamera()
        {
            nint display = SceneWindow_UpdateDisplay(NativePtr);
            m_SceneViewCamera.SetCustomTargetDisplay(display);
            RenderPipeline.Render(m_SceneViewCamera, m_GridMaterial);
        }

        private void TravelScene()
        {
            Transform transform = m_SceneViewCamera.gameObject.transform;

            Vector3 position = transform.Position;
            Quaternion rotation = transform.Rotation;
            SceneWindow_TravelScene(NativePtr, &position, &rotation);
            transform.Position = position;
            transform.Rotation = rotation;
        }

        #region Bindings

        [NativeFunction]
        private static partial nint SceneWindow_New();

        [NativeFunction]
        private static partial void SceneWindow_Delete(nint w);

        [NativeFunction]
        private static partial void SceneWindow_DrawMenuBar(nint w, bool* wireframe);

        [NativeFunction]
        private static partial nint SceneWindow_UpdateDisplay(nint w);

        [NativeFunction]
        private static partial void SceneWindow_DrawSceneView(nint w);

        [NativeFunction]
        private static partial void SceneWindow_TravelScene(nint w, Vector3* cameraPosition, Quaternion* cameraRotation);

        [NativeFunction]
        private static partial float SceneWindow_GetMouseSensitivity(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetMouseSensitivity(nint w, float value);

        [NativeFunction]
        private static partial float SceneWindow_GetRotateDegSpeed(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetRotateDegSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneWindow_GetNormalMoveSpeed(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetNormalMoveSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneWindow_GetFastMoveSpeed(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetFastMoveSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneWindow_GetPanSpeed(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetPanSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneWindow_GetZoomSpeed(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetZoomSpeed(nint w, float value);

        #endregion
    }
}
