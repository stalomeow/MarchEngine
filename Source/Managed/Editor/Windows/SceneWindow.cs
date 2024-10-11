using March.Core;
using March.Core.Binding;
using March.Core.IconFonts;
using March.Core.Rendering;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    enum SceneGizmoOperation
    {
        Pan = 0,
        Translate = 1,
        Rotate = 2,
        Scale = 3
    }

    enum SceneGizmoMode
    {
        Local = 0,
        World = 1
    }

    enum SceneWindowMode
    {
        SceneView = 0,
        Settings = 1
    }

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
        public bool CameraEnableWireframe
        {
            get => m_SceneViewCamera.EnableWireframe;
            set => m_SceneViewCamera.EnableWireframe = value;
        }

        [JsonProperty]
        public float CameraVerticalFieldOfView
        {
            get => m_SceneViewCamera.VerticalFieldOfView;
            set => m_SceneViewCamera.VerticalFieldOfView = value;
        }

        [JsonProperty]
        public float CameraNearClipPlane
        {
            get => m_SceneViewCamera.NearClipPlane;
            set => m_SceneViewCamera.NearClipPlane = value;
        }

        [JsonProperty]
        public float CameraFarClipPlane
        {
            get => m_SceneViewCamera.FarClipPlane;
            set => m_SceneViewCamera.FarClipPlane = value;
        }

        [JsonProperty]
        public bool EnableMSAA
        {
            get => SceneWindow_GetEnableMSAA(NativePtr);
            set => SceneWindow_SetEnableMSAA(NativePtr, value);
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

        [JsonProperty]
        public SceneGizmoOperation GizmoOperation
        {
            get => SceneWindow_GetGizmoOperation(NativePtr);
            set => SceneWindow_SetGizmoOperation(NativePtr, value);
        }

        [JsonProperty]
        public SceneGizmoMode GizmoMode
        {
            get => SceneWindow_GetGizmoMode(NativePtr);
            set => SceneWindow_SetGizmoMode(NativePtr, value);
        }

        [JsonProperty]
        public bool GizmoSnap
        {
            get => SceneWindow_GetGizmoSnap(NativePtr);
            set => SceneWindow_SetGizmoSnap(NativePtr, value);
        }

        [JsonProperty]
        public SceneWindowMode WindowMode
        {
            get => SceneWindow_GetWindowMode(NativePtr);
            set => SceneWindow_SetWindowMode(NativePtr, value);
        }

        protected override void OnDraw()
        {
            base.OnDraw();
            SceneWindow_DrawMenuBar(NativePtr);

            if (WindowMode == SceneWindowMode.SceneView)
            {
                RenderCamera();
                SceneWindow_DrawSceneView(NativePtr);
                TravelScene();
                ManipulateTransform();
            }
            else
            {
                SceneWindow_DrawWindowSettings(NativePtr);

                EditorGUI.SeparatorText("Camera Settings");
                EditorGUI.ObjectPropertyFields(m_SceneViewCamera);
            }
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

        private void ManipulateTransform()
        {
            if (Selection.Active is not GameObject go)
            {
                return;
            }

            Transform transform = go.transform;
            Matrix4x4 m = transform.LocalToWorldMatrix;

            if (SceneWindow_ManipulateTransform(NativePtr, m_SceneViewCamera.NativePtr, &m))
            {
                transform.LocalToWorldMatrix = m;
            }
        }

        #region Bindings

        [NativeFunction]
        private static partial nint SceneWindow_New();

        [NativeFunction]
        private static partial void SceneWindow_Delete(nint w);

        [NativeFunction]
        private static partial void SceneWindow_DrawMenuBar(nint w);

        [NativeFunction]
        private static partial nint SceneWindow_UpdateDisplay(nint w);

        [NativeFunction]
        private static partial void SceneWindow_DrawSceneView(nint w);

        [NativeFunction]
        private static partial void SceneWindow_TravelScene(nint w, Vector3* cameraPosition, Quaternion* cameraRotation);

        [NativeFunction]
        private static partial bool SceneWindow_ManipulateTransform(nint w, nint camera, Matrix4x4* transform);

        [NativeFunction]
        private static partial void SceneWindow_DrawWindowSettings(nint w);

        [NativeFunction]
        private static partial bool SceneWindow_GetEnableMSAA(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetEnableMSAA(nint w, bool value);

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

        [NativeFunction]
        private static partial SceneGizmoOperation SceneWindow_GetGizmoOperation(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetGizmoOperation(nint w, SceneGizmoOperation value);

        [NativeFunction]
        private static partial SceneGizmoMode SceneWindow_GetGizmoMode(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetGizmoMode(nint w, SceneGizmoMode value);

        [NativeFunction]
        private static partial bool SceneWindow_GetGizmoSnap(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetGizmoSnap(nint w, bool value);

        [NativeFunction]
        private static partial SceneWindowMode SceneWindow_GetWindowMode(nint w);

        [NativeFunction]
        private static partial void SceneWindow_SetWindowMode(nint w, SceneWindowMode value);

        #endregion
    }
}
