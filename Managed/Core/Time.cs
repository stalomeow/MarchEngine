using DX12Demo.Core.Binding;

namespace DX12Demo.Core
{
    public static partial class Time
    {
        public static float Delta => Application_GetDeltaTime();

        public static float Elapsed => Application_GetElapsedTime();

        [NativeFunction]
        private static partial float Application_GetDeltaTime();

        [NativeFunction]
        private static partial float Application_GetElapsedTime();
    }
}
