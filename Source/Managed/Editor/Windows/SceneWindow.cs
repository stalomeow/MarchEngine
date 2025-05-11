using March.Core;
using March.Core.IconFont;
using March.Core.Interop;
using March.Core.Rendering;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace March.Editor.Windows
{
    internal enum SceneGizmoOperation
    {
        Pan = 0,
        Translate = 1,
        Rotate = 2,
        Scale = 3
    }

    internal enum SceneGizmoMode
    {
        Local = 0,
        World = 1
    }

    internal enum SceneWindowMode
    {
        SceneView = 0,
        Settings = 1
    }

    [EditorWindowMenu("Window/General/Scene")]
    internal unsafe partial class SceneWindow : EditorWindow
    {
        private readonly Scene m_DummyScene = new();
        private readonly Camera m_SceneViewCamera;

        public SceneWindow() : base(New(), FontAwesome6.VectorSquare, "Scene")
        {
            DefaultSize = new Vector2(960.0f, 540.0f);

            GameObject go = m_DummyScene.CreateGameObject("EditorSceneCamera");
            go.transform.Position = new Vector3(0, 5, -5);
            go.transform.Rotation = Quaternion.CreateFromAxisAngle(Vector3.UnitX, MathF.PI / 4.0f);

            m_SceneViewCamera = go.AddComponent<Camera>();
            m_SceneViewCamera.EnableGizmos = true;
        }

        protected override void OnClose()
        {
            m_DummyScene.Dispose();

            base.OnClose();
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
        [NativeProperty]
        public partial bool EnableMSAA { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float MouseSensitivity { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float RotateDegSpeed { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float NormalMoveSpeed { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float FastMoveSpeed { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float PanSpeed { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float ZoomSpeed { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial SceneGizmoOperation GizmoOperation { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial SceneGizmoMode GizmoMode { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial bool GizmoSnap { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial Vector3 GizmoTranslationSnapValue { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float GizmoRotationSnapValue { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float GizmoScaleSnapValue { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial SceneWindowMode WindowMode { get; set; }

        protected override void OnDraw()
        {
            base.OnDraw();
            DrawMenuBar();

            m_SceneViewCamera.SetCustomTargetDisplay(UpdateDisplay());

            if (WindowMode == SceneWindowMode.SceneView)
            {
                DrawSceneView();
                DrawGizmosGUI();
                ManipulateTransform();
                TravelScene();
                HandleDragDrop();
            }
            else
            {
                DrawWindowSettings();

                if (EditorGUI.Foldout("Camera", string.Empty))
                {
                    using (new EditorGUI.IndentedScope(2))
                    {
                        EditorGUI.ObjectPropertyFields(m_SceneViewCamera);
                    }
                }
            }
        }

        private void TravelScene()
        {
            Transform transform = m_SceneViewCamera.gameObject.transform;

            Vector3 position = transform.Position;
            Quaternion rotation = transform.Rotation;
            TravelScene(ref position, ref rotation);
            transform.Position = position;
            transform.Rotation = rotation;
        }

        private void ManipulateTransform()
        {
            GameObject? go = Selection.FirstOrDefault<GameObject>();

            if (go == null)
            {
                return;
            }

            Transform transform = go.transform;
            Matrix4x4 m = transform.LocalToWorldMatrix;

            if (ManipulateTransform(m_SceneViewCamera, ref m))
            {
                transform.LocalToWorldMatrix = m;
            }
        }

        private void DrawGizmosGUI()
        {
            try
            {
                BeginGizmosGUI(m_SceneViewCamera);
                SceneManager.CurrentScene.DrawGizmosGUI(selected: Selection.Contains);
            }
            finally
            {
                EndGizmosGUI();
            }
        }

        private static void HandleDragDrop()
        {
            if (!DragDrop.BeginTarget(DragDropArea.Window, out DragDropData data))
            {
                return;
            }

            if (data.Payload is GameObject go)
            {
                if (data.IsDelivery)
                {
                    SceneManager.CurrentScene.AddRootGameObject(Instantiate(go));
                }

                DragDrop.EndTarget(DragDropResult.AcceptByRect);
            }
            else
            {
                DragDrop.EndTarget(DragDropResult.Reject);
            }
        }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void DrawMenuBar();

        [NativeMethod]
        private partial nint UpdateDisplay();

        [NativeMethod]
        private partial void DrawSceneView();

        [NativeMethod]
        private partial void TravelScene(ref Vector3 cameraPosition, ref Quaternion cameraRotation);

        [NativeMethod]
        private partial bool ManipulateTransform(Camera camera, ref Matrix4x4 transform);

        [NativeMethod]
        private partial void BeginGizmosGUI(Camera camera);

        [NativeMethod]
        private partial void EndGizmosGUI();

        [NativeMethod]
        private partial void DrawWindowSettings();
    }
}
