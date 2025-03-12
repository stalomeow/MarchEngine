using March.Core;
using March.Core.IconFont;
using March.Core.Interop;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.AssetPipeline;
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
        private readonly Material m_GridMaterial = new();
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

            m_GridMaterial.Shader = AssetDatabase.Load<Shader>("Engine/Shaders/SceneViewGrid.shader")!;
        }

        protected override void OnClose()
        {
            m_GridMaterial.Shader = null;
            m_GridMaterial.Dispose();
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
        public Color GridXAxisColor
        {
            get => m_GridMaterial.MustGetColor("_XAxisColor");
            set => m_GridMaterial.SetColor("_XAxisColor", value);
        }

        [JsonProperty]
        public Color GridZAxisColor
        {
            get => m_GridMaterial.MustGetColor("_ZAxisColor");
            set => m_GridMaterial.SetColor("_ZAxisColor", value);
        }

        [JsonProperty]
        public Color GridLineColor
        {
            get => m_GridMaterial.MustGetColor("_LineColor");
            set => m_GridMaterial.SetColor("_LineColor", value);
        }

        [JsonProperty]
        [FloatDrawer(Min = 0.0f, Max = 1.0f, Slider = true)]
        public float GridAntialiasing
        {
            get => m_GridMaterial.MustGetFloat("_Antialiasing");
            set => m_GridMaterial.SetFloat("_Antialiasing", value);
        }

        [JsonProperty]
        [FloatDrawer(Min = 0.0f, Max = 1.0f, Slider = true)]
        public float GridFadeOut
        {
            get => m_GridMaterial.MustGetFloat("_FadeOut");
            set => m_GridMaterial.SetFloat("_FadeOut", value);
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

            if (WindowMode == SceneWindowMode.SceneView)
            {
                ManipulateTransform();
                TravelScene();
                HandleDragDrop();
                DrawScene();
                DrawGizmosGUI();
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

                EditorGUI.Space();

                if (EditorGUI.Foldout("Grid", string.Empty))
                {
                    using (new EditorGUI.IndentedScope(2))
                    {
                        var contract = (JsonObjectContract)PersistentManager.ResolveJsonContract(GetType());
                        EditorGUI.PropertyField(contract.GetEditorProperty(this, nameof(GridXAxisColor)));
                        EditorGUI.PropertyField(contract.GetEditorProperty(this, nameof(GridZAxisColor)));
                        EditorGUI.PropertyField(contract.GetEditorProperty(this, nameof(GridLineColor)));
                        EditorGUI.PropertyField(contract.GetEditorProperty(this, nameof(GridAntialiasing)));
                        EditorGUI.PropertyField(contract.GetEditorProperty(this, nameof(GridFadeOut)));
                    }
                }
            }
        }

        private void DrawScene()
        {
            m_SceneViewCamera.SetCustomTargetDisplay(UpdateDisplay());
            RenderPipeline.Render(m_SceneViewCamera, m_GridMaterial);
            DrawSceneView();
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
            if (Selection.Active is not GameObject go)
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
                SceneManager.CurrentScene.DrawGizmosGUI(selected: go => Selection.Active == go);
            }
            finally
            {
                EndGizmosGUI();
            }
        }

        private static void HandleDragDrop()
        {
            if (!DragDrop.BeginTarget(DragDropArea.Window, out MarchObject? payload, out bool isDelivery))
            {
                return;
            }

            if (payload is GameObject go)
            {
                if (isDelivery)
                {
                    SceneManager.CurrentScene.AddRootGameObject(Instantiate(go));
                }

                DragDrop.EndTarget(DragDropResult.Accept);
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
