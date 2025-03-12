using March.Core.IconFont;
using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Rendering
{
    public partial class Camera : Component
    {
        public Camera() : base(New()) { }

        [NativeProperty]
        public partial int PixelWidth { get; }

        [NativeProperty]
        public partial int PixelHeight { get; }

        [NativeProperty]
        public partial float AspectRatio { get; }

        [NativeProperty]
        public partial bool EnableMSAA { get; }

        [JsonProperty]
        [InspectorName("Wireframe")]
        [NativeProperty]
        public partial bool EnableWireframe { get; set; }

        [JsonProperty]
        [InspectorName("Field Of View")]
        [Tooltip("The vertical field of view.")]
        [FloatDrawer(Min = 1.0f, Max = 179.0f, Slider = true)] // 和 C++ 里保持一致
        [NativeProperty]
        public partial float VerticalFieldOfView { get; set; }

        [NativeProperty]
        public partial float HorizontalFieldOfView { get; set; }

        [JsonProperty]
        [InspectorName("Near")]
        [Tooltip("The near clip plane.")]
        [FloatDrawer(Min = 0.001f)] // 和 C++ 里保持一致
        [NativeProperty]
        public partial float NearClipPlane { get; set; }

        [JsonProperty]
        [InspectorName("Far")]
        [Tooltip("The far clip plane.")]
        [FloatDrawer(Min = 0.001f)] // 和 C++ 里保持一致
        [NativeProperty]
        public partial float FarClipPlane { get; set; }

        [NativeProperty]
        internal partial bool EnableGizmos { get; set; }

        [NativeProperty]
        public partial uint TAAFrameIndex { get; }

        [NativeProperty]
        public partial Matrix4x4 ViewMatrix { get; }

        [NativeProperty]
        public partial Matrix4x4 ProjectionMatrix { get; }

        [NativeProperty]
        public partial Matrix4x4 ViewProjectionMatrix { get; }

        [NativeProperty]
        public partial Matrix4x4 NonJitteredProjectionMatrix { get; }

        [NativeProperty]
        public partial Matrix4x4 NonJitteredViewProjectionMatrix { get; }

        [NativeProperty]
        public partial Matrix4x4 PrevNonJitteredViewProjectionMatrix { get; }

        [NativeMethod]
        internal partial void SetCustomTargetDisplay(nint display);

        internal void ResetTargetDisplay() => SetCustomTargetDisplay(nint.Zero);

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

        [NativeMethod]
        private static partial nint New();
    }
}
