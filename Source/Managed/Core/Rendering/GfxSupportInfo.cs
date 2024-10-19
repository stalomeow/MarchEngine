using March.Core.Binding;

namespace March.Core.Rendering
{
    public enum GfxColorSpace
    {
        Linear,
        Gamma,
    }

    public static partial class GfxSupportInfo
    {
        public static bool UseReversedZBuffer => GfxSupportInfo_UseReversedZBuffer();

        public static GfxColorSpace ColorSpace => GfxSupportInfo_GetColorSpace();

        #region Bindings

        [NativeFunction]
        private static partial bool GfxSupportInfo_UseReversedZBuffer();

        [NativeFunction]
        private static partial GfxColorSpace GfxSupportInfo_GetColorSpace();

        #endregion
    }
}
