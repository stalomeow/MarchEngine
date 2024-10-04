using March.Core.Binding;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core
{
    public partial class Camera : Component
    {
        public Camera() : base(Camera_New()) { }

        protected override void DisposeNative()
        {
            Camera_Delete(NativePtr);
            base.DisposeNative();
        }

        public float PixelWidth => Camera_GetPixelWidth(NativePtr);

        public float PixelHeight => Camera_GetPixelHeight(NativePtr);

        public float AspectRatio => Camera_GetAspectRatio(NativePtr);

        [JsonProperty]
        [InspectorName("MSAA")]
        public bool EnableMSAA
        {
            get => Camera_GetEnableMSAA(NativePtr);
            set => Camera_SetEnableMSAA(NativePtr, value);
        }

        [JsonProperty]
        [InspectorName("Wireframe")]
        public bool EnableWireframe
        {
            get => Camera_GetEnableWireframe(NativePtr);
            set => Camera_SetEnableWireframe(NativePtr, value);
        }

        [JsonProperty]
        [InspectorName("Field Of View")]
        [Tooltip("The vertical field of view.")]
        [FloatDrawer(Min = 1.0f, Max = 179.0f, Slider = true)] // 和 C++ 里保持一致
        public float VerticalFieldOfView
        {
            get => Camera_GetVerticalFieldOfView(NativePtr);
            set => Camera_SetVerticalFieldOfView(NativePtr, value);
        }

        public float HorizontalFieldOfView
        {
            get => Camera_GetHorizontalFieldOfView(NativePtr);
            set => Camera_SetHorizontalFieldOfView(NativePtr, value);
        }

        [JsonProperty]
        [InspectorName("Near")]
        [Tooltip("The near clip plane.")]
        [FloatDrawer(Min = 0.001f)] // 和 C++ 里保持一致
        public float NearClipPlane
        {
            get => Camera_GetNearClipPlane(NativePtr);
            set => Camera_SetNearClipPlane(NativePtr, value);
        }

        [JsonProperty]
        [InspectorName("Far")]
        [Tooltip("The far clip plane.")]
        [FloatDrawer(Min = 0.001f)] // 和 C++ 里保持一致
        public float FarClipPlane
        {
            get => Camera_GetFarClipPlane(NativePtr);
            set => Camera_SetFarClipPlane(NativePtr, value);
        }

        internal bool IsEditorSceneCamera
        {
            get => Camera_GetIsEditorSceneCamera(NativePtr);
            set => Camera_SetIsEditorSceneCamera(NativePtr, value);
        }

        public Matrix4x4 ViewMatrix => Camera_GetViewMatrix(NativePtr);

        public Matrix4x4 ProjectionMatrix => Camera_GetProjectionMatrix(NativePtr);

        #region Bindings

        [NativeFunction]
        private static partial nint Camera_New();

        [NativeFunction]
        private static partial void Camera_Delete(nint camera);

        [NativeFunction]
        private static partial int Camera_GetPixelWidth(nint camera);

        [NativeFunction]
        private static partial int Camera_GetPixelHeight(nint camera);

        [NativeFunction]
        private static partial float Camera_GetAspectRatio(nint camera);

        [NativeFunction]
        private static partial bool Camera_GetEnableMSAA(nint camera);

        [NativeFunction]
        private static partial void Camera_SetEnableMSAA(nint camera, bool value);

        [NativeFunction]
        private static partial float Camera_GetVerticalFieldOfView(nint camera);

        [NativeFunction]
        private static partial void Camera_SetVerticalFieldOfView(nint camera, float value);

        [NativeFunction]
        private static partial float Camera_GetHorizontalFieldOfView(nint camera);

        [NativeFunction]
        private static partial void Camera_SetHorizontalFieldOfView(nint camera, float value);

        [NativeFunction]
        private static partial float Camera_GetNearClipPlane(nint camera);

        [NativeFunction]
        private static partial void Camera_SetNearClipPlane(nint camera, float value);

        [NativeFunction]
        private static partial float Camera_GetFarClipPlane(nint camera);

        [NativeFunction]
        private static partial void Camera_SetFarClipPlane(nint camera, float value);

        [NativeFunction]
        private static partial bool Camera_GetEnableWireframe(nint camera);

        [NativeFunction]
        private static partial void Camera_SetEnableWireframe(nint camera, bool value);

        [NativeFunction]
        private static partial bool Camera_GetIsEditorSceneCamera(nint camera);

        [NativeFunction]
        private static partial void Camera_SetIsEditorSceneCamera(nint camera, bool value);

        [NativeFunction]
        private static partial Matrix4x4 Camera_GetViewMatrix(nint camera);

        [NativeFunction]
        private static partial Matrix4x4 Camera_GetProjectionMatrix(nint camera);

        #endregion
    }
}
