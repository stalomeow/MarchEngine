using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using System.Collections;
using System.Runtime.InteropServices;

namespace March.Editor
{
    public enum DragDropArea
    {
        Item,
        Window,
    }

    public enum DragDropResult
    {
        /// <summary>
        /// 忽略
        /// </summary>
        Ignore,

        /// <summary>
        /// 拒绝
        /// </summary>
        Reject,

        /// <summary>
        /// 接受并显示一个矩形框
        /// </summary>
        AcceptByRect,

        /// <summary>
        /// 接受并显示一条线
        /// </summary>
        AcceptByLine,
    }

    public static partial class DragDrop
    {
        public static List<MarchObject> Objects { get; } = [];

        /// <summary>
        /// 从外部拖拽的文件路径列表
        /// </summary>
        public static List<string> Paths { get; } = [];

        /// <summary>
        /// 可以保存额外的数据
        /// </summary>
        public static List<object> Extras { get; } = [];

        /// <summary>
        /// 上下文对象，可以判断拖拽的来源
        /// </summary>
        public static object? Context { get; set; }

        /// <summary>
        /// 用户释放鼠标按键时为 <see langword="true"/>，此时执行真正的接受操作，只在 <see cref="BeginTarget(DragDropArea)"/> 与 <see cref="EndTarget(DragDropResult)"/> 范围内有效
        /// </summary>
        public static bool IsDelivery { get; private set; }

        [NativeProperty]
        public static partial bool IsActive { get; }

        static DragDrop()
        {
            Application.OnTick += () =>
            {
                if (!IsActive)
                {
                    ClearData();
                }
            };
        }

        public static void ClearData()
        {
            Objects.Clear();
            Paths.Clear();
            Extras.Clear();
            Context = null;
        }

        public static bool BeginSource()
        {
            return BeginSource(isExternal: false);
        }

        private static bool BeginSource(bool isExternal)
        {
            if (BeginSourceNative(isExternal))
            {
                ClearData();
                return true;
            }
            return false;
        }

        [NativeMethod("BeginSource")]
        private static partial bool BeginSourceNative(bool isExternal);

        [NativeMethod]
        public static partial void EndSource(StringLike tooltip);

        public static bool BeginTarget(DragDropArea area)
        {
            if (!BeginTargetNative(area == DragDropArea.Window))
            {
                return false;
            }

            if (!CheckPayloadNative(out bool isDelivery))
            {
                EndTargetNative();
                return false;
            }

            IsDelivery = isDelivery;
            return true;
        }

        public static void EndTarget(DragDropResult result)
        {
            AcceptTargetNative(result);
            EndTargetNative();
        }

        [NativeMethod("BeginTarget")]
        private static partial bool BeginTargetNative(bool useWindow);

        [NativeMethod("CheckPayload")]
        private static partial bool CheckPayloadNative(out bool isDelivery);

        [NativeMethod("AcceptTarget")]
        private static partial void AcceptTargetNative(DragDropResult result);

        [NativeMethod("EndTarget")]
        private static partial void EndTargetNative();

        private static int? s_ExternalFileCoroutineId = null;

        [UnmanagedCallersOnly]
        private static unsafe void HandleExternalFiles(nint hDrop)
        {
            uint fileCount = DragQueryFile(hDrop, uint.MaxValue, nint.Zero, 0);

            if (fileCount == 0)
            {
                return;
            }

            List<string> paths = ListPool<string>.Shared.Rent();
            using var buffer = ListPool<char>.Get();

            for (uint i = 0; i < fileCount; i++)
            {
                uint len = DragQueryFile(hDrop, i, nint.Zero, 0) + 1; // 需要添加一个 '\0'

                if (buffer.Value.Count < len)
                {
                    CollectionsMarshal.SetCount(buffer.Value, (int)len);
                }

                fixed (char* p = CollectionsMarshal.AsSpan(buffer.Value))
                {
                    _ = DragQueryFile(hDrop, i, (nint)p, len);
                    paths.Add(new string(p));
                }
            }

            if (s_ExternalFileCoroutineId != null)
            {
                Application.StopCoroutine(s_ExternalFileCoroutineId.Value);
            }

            // BeginSource/EndSource 需要 ImGui 的 Context，不可以在这里调用，开一个协程来处理
            s_ExternalFileCoroutineId = Application.StartCoroutine(UpdateExternalFileSource(paths));
        }

        private static IEnumerator UpdateExternalFileSource(List<string> paths)
        {
            try
            {
                bool firstTime = true;

                while (firstTime || IsActive)
                {
                    if (BeginSource(isExternal: true))
                    {
                        firstTime = false;
                        Paths.AddRange(paths);

                        using var tooltip = StringBuilderPool.Get();
                        tooltip.Value.Append(Paths.Count).Append(" External File");

                        if (Paths.Count > 1)
                        {
                            tooltip.Value.Append('s');
                        }

                        EndSource(tooltip);
                    }

                    yield return null;
                }
            }
            finally
            {
                ListPool<string>.Shared.Return(paths);
            }
        }

        // https://learn.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-dragqueryfilew
        [DllImport("shell32.dll", CharSet = CharSet.Unicode)]
        private static extern uint DragQueryFile(nint hDrop, uint iFile, nint lpszFile, uint cch);
    }
}
