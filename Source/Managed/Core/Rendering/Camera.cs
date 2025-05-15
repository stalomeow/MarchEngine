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

        [NativeMethod]
        private static partial nint New();
    }
}
