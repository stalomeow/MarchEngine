using March.Core.Interop;

namespace March.Core.Rendering
{
    public enum GraphicsColorSpace
    {
        Linear,
        Gamma,
    }

    public static partial class GraphicsSettings
    {
        public static bool UseReversedZBuffer => GfxSettings_UseReversedZBuffer();

        public static GraphicsColorSpace ColorSpace => GfxSettings_GetColorSpace();

        #region Bindings

        [NativeFunction]
        private static partial bool GfxSettings_UseReversedZBuffer();

        [NativeFunction]
        private static partial GraphicsColorSpace GfxSettings_GetColorSpace();

        #endregion
    }
}
