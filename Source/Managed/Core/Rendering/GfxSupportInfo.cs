using March.Core.Binding;

namespace March.Core.Rendering
{
    public static partial class GfxSupportInfo
    {
        public static bool UseReversedZBuffer => GfxSupportInfo_UseReversedZBuffer();

        #region Bindings

        [NativeFunction]
        private static partial bool GfxSupportInfo_UseReversedZBuffer();

        #endregion
    }
}
