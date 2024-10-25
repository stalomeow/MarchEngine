using March.Core.Binding;
using March.Core.IconFonts;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Rendering
{
    public partial class Camera : Component
    {
        public Camera() : base(Camera_New()) { }

        protected override void DisposeNative()
        {
            Camera_Delete(NativePtr);
            base.DisposeNative();
        }

        public int PixelWidth => Camera_GetPixelWidth(NativePtr);

        public int PixelHeight => Camera_GetPixelHeight(NativePtr);

        public float AspectRatio => Camera_GetAspectRatio(NativePtr);

        public bool EnableMSAA => Camera_GetEnableMSAA(NativePtr);

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

        internal bool EnableGizmos
        {
            get => Camera_GetEnableGizmos(NativePtr);
            set => Camera_SetEnableGizmos(NativePtr, value);
        }

        public Matrix4x4 ViewMatrix => Camera_GetViewMatrix(NativePtr);

        public Matrix4x4 ProjectionMatrix => Camera_GetProjectionMatrix(NativePtr);

        public Matrix4x4 ViewProjectionMatrix => Camera_GetViewProjectionMatrix(NativePtr);

        internal void SetCustomTargetDisplay(nint display)
        {
            Camera_SetCustomTargetDisplay(NativePtr, display);
        }

        internal void ResetTargetDisplay()
        {
            SetCustomTargetDisplay(nint.Zero);
        }

        protected override void OnDrawGizmos(bool isSelected)
        {
            base.OnDrawGizmos(isSelected);

            using (new Gizmos.ColorScope(isSelected ? Color.White : new Color(1, 1, 1, 0.6f)))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(transform)))
            {
                float tanHalfVerticalFov = MathF.Tan(float.DegreesToRadians(VerticalFieldOfView * 0.5f));
                float tanHalfHorizontalFov = MathF.Tan(float.DegreesToRadians(HorizontalFieldOfView * 0.5f));

                Vector3 forwardNear = NearClipPlane * Vector3.UnitZ;
                Vector3 forwardFar = FarClipPlane * Vector3.UnitZ;
                Vector3 upNear = tanHalfVerticalFov * NearClipPlane * Vector3.UnitY;
                Vector3 upFar = tanHalfVerticalFov * FarClipPlane * Vector3.UnitY;
                Vector3 rightNear = tanHalfHorizontalFov * NearClipPlane * Vector3.UnitX;
                Vector3 rightFar = tanHalfHorizontalFov * FarClipPlane * Vector3.UnitX;

                Gizmos.DrawLine(forwardNear + upNear - rightNear, forwardNear + upNear + rightNear);
                Gizmos.DrawLine(forwardNear - upNear - rightNear, forwardNear - upNear + rightNear);
                Gizmos.DrawLine(forwardNear + upNear - rightNear, forwardNear - upNear - rightNear);
                Gizmos.DrawLine(forwardNear + upNear + rightNear, forwardNear - upNear + rightNear);

                Gizmos.DrawLine(forwardFar + upFar - rightFar, forwardFar + upFar + rightFar);
                Gizmos.DrawLine(forwardFar - upFar - rightFar, forwardFar - upFar + rightFar);
                Gizmos.DrawLine(forwardFar + upFar - rightFar, forwardFar - upFar - rightFar);
                Gizmos.DrawLine(forwardFar + upFar + rightFar, forwardFar - upFar + rightFar);

                Gizmos.DrawLine(forwardNear - upNear - rightNear, forwardFar - upFar - rightFar);
                Gizmos.DrawLine(forwardNear + upNear - rightNear, forwardFar + upFar - rightFar);
                Gizmos.DrawLine(forwardNear - upNear + rightNear, forwardFar - upFar + rightFar);
                Gizmos.DrawLine(forwardNear + upNear + rightNear, forwardFar + upFar + rightFar);
            }
        }

        protected override void OnDrawGizmosGUI(bool isSelected)
        {
            base.OnDrawGizmosGUI(isSelected);

            // Icon
            Gizmos.DrawText(gameObject.transform.Position, FontAwesome6.Video);
        }

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
        private static partial bool Camera_GetEnableGizmos(nint camera);

        [NativeFunction]
        private static partial void Camera_SetEnableGizmos(nint camera, bool value);

        [NativeFunction]
        private static partial Matrix4x4 Camera_GetViewMatrix(nint camera);

        [NativeFunction]
        private static partial Matrix4x4 Camera_GetProjectionMatrix(nint camera);

        [NativeFunction]
        private static partial Matrix4x4 Camera_GetViewProjectionMatrix(nint camera);

        [NativeFunction]
        private static partial void Camera_SetCustomTargetDisplay(nint camera, nint display);

        #endregion
    }
}
