using March.Core;
using March.Core.Binding;
using March.Core.Rendering;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    internal unsafe partial class SceneWindow : EditorWindow
    {
        private readonly Material m_GridMaterial = new();
        private readonly Scene m_DummyScene = new();
        private readonly Camera m_SceneViewCamera;

        public SceneWindow() : base(SceneWindow_New(), "Scene")
        {
            GameObject go = m_DummyScene.CreateGameObject("EditorSceneCamera");
            go.transform.Position = new Vector3(0, 5, -5);
            go.transform.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitX, MathF.PI / 4.0f);

            m_SceneViewCamera = go.AddComponent<Camera>();
            m_SceneViewCamera.EnableGizmos = true;

            m_GridMaterial.Shader = AssetDatabase.Load<Shader>("Engine/Shaders/SceneViewGrid.shader")!;
            RenderPipeline.SetSceneViewGridMaterial(m_GridMaterial); // TODO remove it
        }

        protected override void OnDispose(bool disposing)
        {
            // TODO dispose scene

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
            get => SceneView_GetMouseSensitivity(NativePtr);
            set => SceneView_SetMouseSensitivity(NativePtr, value);
        }

        [JsonProperty]
        public float RotateDegSpeed
        {
            get => SceneView_GetRotateDegSpeed(NativePtr);
            set => SceneView_SetRotateDegSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float NormalMoveSpeed
        {
            get => SceneView_GetNormalMoveSpeed(NativePtr);
            set => SceneView_SetNormalMoveSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float FastMoveSpeed
        {
            get => SceneView_GetFastMoveSpeed(NativePtr);
            set => SceneView_SetFastMoveSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float PanSpeed
        {
            get => SceneView_GetPanSpeed(NativePtr);
            set => SceneView_SetPanSpeed(NativePtr, value);
        }

        [JsonProperty]
        public float ZoomSpeed
        {
            get => SceneView_GetZoomSpeed(NativePtr);
            set => SceneView_SetZoomSpeed(NativePtr, value);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            DrawMenuBar();
            UpdateDisplay();
            SceneWindow_DrawSceneView(NativePtr);
            TravelScene();
        }

        private void DrawMenuBar()
        {
            bool enableWireframe = m_SceneViewCamera.EnableWireframe;
            SceneWindow_DrawMenuBar(NativePtr, &enableWireframe);
            m_SceneViewCamera.EnableWireframe = enableWireframe;
        }

        private void UpdateDisplay()
        {
            nint display = SceneWindow_UpdateDisplay(NativePtr);
            m_SceneViewCamera.SetCustomTargetDisplay(display);
        }

        private void TravelScene()
        {
            Transform transform = m_SceneViewCamera.gameObject.transform;

            Vector3 position = transform.Position;
            Quaternion rotation = transform.Rotation;
            SceneView_TravelScene(NativePtr, &position, &rotation);
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
        private static partial void SceneView_TravelScene(nint w, Vector3* cameraPosition, Quaternion* cameraRotation);

        [NativeFunction]
        private static partial float SceneView_GetMouseSensitivity(nint w);

        [NativeFunction]
        private static partial void SceneView_SetMouseSensitivity(nint w, float value);

        [NativeFunction]
        private static partial float SceneView_GetRotateDegSpeed(nint w);

        [NativeFunction]
        private static partial void SceneView_SetRotateDegSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneView_GetNormalMoveSpeed(nint w);

        [NativeFunction]
        private static partial void SceneView_SetNormalMoveSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneView_GetFastMoveSpeed(nint w);

        [NativeFunction]
        private static partial void SceneView_SetFastMoveSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneView_GetPanSpeed(nint w);

        [NativeFunction]
        private static partial void SceneView_SetPanSpeed(nint w, float value);

        [NativeFunction]
        private static partial float SceneView_GetZoomSpeed(nint w);

        [NativeFunction]
        private static partial void SceneView_SetZoomSpeed(nint w, float value);

        #endregion
    }
}
