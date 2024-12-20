using March.Core.Interop;
using System.Runtime.InteropServices;

#pragma warning disable IDE0051 // Remove unused private members

namespace March.Core
{
    public partial class Application
    {
        public static event Action? OnTick;

        #region OnQuit

        private static Action? s_OnQuitAction;

        public static event Action? OnQuit
        {
            add => s_OnQuitAction = value + s_OnQuitAction; // 退出时要从后往前调用
            remove => s_OnQuitAction -= value;
        }

        #endregion

        #region DataPath

        private static string? s_CachedDataPath;

        public static string DataPath => s_CachedDataPath ??= GetDataPath();

        [NativeMethod]
        private static partial string GetDataPath();

        #endregion

        [UnmanagedCallersOnly]
        private static void Initialize()
        {
            SceneManager.Initialize();
        }

        [UnmanagedCallersOnly]
        private static void Tick() => OnTick?.Invoke();

        [UnmanagedCallersOnly]
        private static void Quit() => s_OnQuitAction?.Invoke();

        [UnmanagedCallersOnly]
        private static void FullGC()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
        }
    }
}
