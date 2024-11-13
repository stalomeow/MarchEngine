using March.Core.Interop;

namespace March.Core
{
    public static partial class Time
    {
        public static float Delta => Application_GetDeltaTime();

        public static float Elapsed => Application_GetElapsedTime();

        public static ulong FrameCount => Application_GetFrameCount();

        #region Bindings

        [NativeMethod]
        private static partial float Application_GetDeltaTime();

        [NativeMethod]
        private static partial float Application_GetElapsedTime();

        [NativeMethod]
        private static partial ulong Application_GetFrameCount();

        #endregion
    }
}
