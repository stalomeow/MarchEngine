using March.Core.Binding;

namespace March.Core
{
    public static partial class Time
    {
        public static float Delta => Application_GetDeltaTime();

        public static float Elapsed => Application_GetElapsedTime();

        public static ulong FrameCount => Application_GetFrameCount();

        [NativeFunction]
        private static partial float Application_GetDeltaTime();

        [NativeFunction]
        private static partial float Application_GetElapsedTime();

        [NativeFunction]
        private static partial ulong Application_GetFrameCount();
    }
}
