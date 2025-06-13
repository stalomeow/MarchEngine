using March.Core.Interop;

namespace March.Core
{
    [NativeTypeName("Application")]
    public static partial class Time
    {
        [NativeProperty("DeltaTime")]
        public static partial float Delta { get; }

        [NativeProperty("ElapsedTime")]
        public static partial float Elapsed { get; }

        [NativeProperty]
        public static partial ulong FrameCount { get; }

        [NativeProperty]
        public static partial uint FPS { get; }
    }
}
