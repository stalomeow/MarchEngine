using March.Core.Interop;
using March.Core.Rendering;
using System.Collections;
using System.Diagnostics;
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
        private static void Tick()
        {
            OnTick?.Invoke();
            UpdateCoroutines();
        }

        [UnmanagedCallersOnly]
        private static void Quit()
        {
            StopAllCoroutines();
            s_OnQuitAction?.Invoke();
        }

        [UnmanagedCallersOnly]
        private static void FullGC()
        {
            // GC 两次，保证有 Finalizer 的对象被回收

            GC.Collect();
            GC.WaitForPendingFinalizers();

            GC.Collect();
            GC.WaitForPendingFinalizers();
        }

        public static void OpenURL(string url)
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = url,
                UseShellExecute = true, // 必须设置为 true 才能在现代 Windows 上打开浏览器
            });
        }

        #region Coroutine

        private struct CoroutineData
        {
            public required int Id;
            public required IEnumerator Routine;
        }

        private static readonly LinkedList<CoroutineData> s_Coroutines = [];
        private static readonly Stack<LinkedListNode<CoroutineData>> s_CoroutineNodePool = [];
        private static readonly Dictionary<int, LinkedListNode<CoroutineData>> s_CoroutineMap = [];
        private static int s_LastCoroutineId = 0;

        public static int StartCoroutine(IEnumerator routine)
        {
            var data = new CoroutineData
            {
                Id = ++s_LastCoroutineId,
                Routine = routine
            };

            if (s_CoroutineNodePool.TryPop(out LinkedListNode<CoroutineData>? node))
            {
                node.Value = data;
            }
            else
            {
                node = new LinkedListNode<CoroutineData>(data);
            }

            s_Coroutines.AddLast(node);
            s_CoroutineMap[data.Id] = node;
            return data.Id;
        }

        public static void StopCoroutine(int id)
        {
            if (s_CoroutineMap.Remove(id, out LinkedListNode<CoroutineData>? node))
            {
                RemoveCoroutineNode(node, updateMap: false);
            }
        }

        public static void StopAllCoroutines()
        {
            foreach (KeyValuePair<int, LinkedListNode<CoroutineData>> kv in s_CoroutineMap)
            {
                RemoveCoroutineNode(kv.Value, updateMap: false);
            }

            s_CoroutineMap.Clear();
        }

        private static void UpdateCoroutines()
        {
            int count = s_Coroutines.Count; // 之后的操作会改变 Count
            LinkedListNode<CoroutineData>? node = s_Coroutines.First;

            for (int i = 0; i < count; i++)
            {
                LinkedListNode<CoroutineData> curr = node!;
                node = curr.Next;

                if (!curr.ValueRef.Routine.MoveNext())
                {
                    RemoveCoroutineNode(curr, updateMap: true);
                }
            }
        }

        private static void RemoveCoroutineNode(LinkedListNode<CoroutineData> node, bool updateMap)
        {
            if (node.ValueRef.Routine is IDisposable disposable)
            {
                disposable.Dispose();
            }

            if (updateMap)
            {
                s_CoroutineMap.Remove(node.ValueRef.Id);
            }

            node.Value = default;
            s_Coroutines.Remove(node);
            s_CoroutineNodePool.Push(node);
        }

        #endregion
    }
}
