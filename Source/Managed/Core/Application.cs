using March.Core.Interop;
using March.Core.Rendering;
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

        #region ProjectName

        private static string? s_CachedProjectName;

        public static string ProjectName => s_CachedProjectName ??= GetProjectName();

        [NativeMethod]
        private static partial string GetProjectName();

        #endregion

        #region DataPath

        private static string? s_CachedDataPath;

        public static string DataPath => s_CachedDataPath ??= GetDataPath();

        [NativeMethod]
        private static partial string GetDataPath();

        #endregion

        #region EngineResourcePath

        private static string? s_CachedEngineResourcePath;

        /// <summary>
        /// 获取引擎内置资源的路径 (Unix Style)
        /// </summary>
        public static string EngineResourcePath => s_CachedEngineResourcePath ??= GetEngineResourcePath();

        [NativeMethod]
        private static partial string GetEngineResourcePath();

        #endregion

        #region EngineShaderPath

        private static string? s_CachedEngineShaderPath;

        /// <summary>
        /// 获取引擎内置 Shader 的路径 (Unix Style)
        /// </summary>
        public static string EngineShaderPath => s_CachedEngineShaderPath ??= GetEngineShaderPath();

        [NativeMethod]
        private static partial string GetEngineShaderPath();

        #endregion

        #region ShaderCachePath

        private static string? s_CachedShaderCachePath;

        public static string ShaderCachePath => s_CachedShaderCachePath ??= GetShaderCachePath();

        [NativeMethod]
        private static partial string GetShaderCachePath();

        #endregion

        [NativeProperty]
        internal static partial bool IsEngineResourceEditable { get; }

        [NativeProperty]
        internal static partial bool IsEngineShaderEditable { get; }

        [UnmanagedCallersOnly]
        private static void Initialize()
        {
            Texture.InitializeDefaults();
            Mesh.InitializeGeometries();
        }

        [UnmanagedCallersOnly]
        private static void PostInitialize()
        {
            RenderPipeline.Initialize();
        }

        [UnmanagedCallersOnly]
        private static void Tick() => OnTick?.Invoke();

        [UnmanagedCallersOnly]
        private static void Quit() => s_OnQuitAction?.Invoke();

        [UnmanagedCallersOnly]
        private static void FullGC()
        {
            // GC 两次，保证有 Finalizer 的对象被回收

            GC.Collect();
            GC.WaitForPendingFinalizers();

            GC.Collect();
            GC.WaitForPendingFinalizers();
        }
    }
}
