using March.Core.Interop;

namespace March.Core.Rendering
{
    public enum GraphicsColorSpace
    {
        Linear,
        Gamma,
    }

    [NativeTypeName("GfxSettings")]
    public static partial class GraphicsSettings
    {
        [NativeProperty]
        public static partial bool UseReversedZBuffer { get; }

        [NativeProperty]
        public static partial GraphicsColorSpace ColorSpace { get; }
    }
}
