using March.Core;
using March.Core.Interop;

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

        public static List<object> Extras { get; } = [];

        public static object? Context { get; set; }

        /// <summary>
        /// 用户释放鼠标按键时为 <see langword="true"/>，只在 <see cref="BeginTarget(DragDropArea)"/> 与 <see cref="EndTarget(DragDropResult)"/> 范围内有效
        /// </summary>
        public static bool IsDelivery { get; private set; }

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
            Extras.Clear();
            Context = null;
        }

        public static bool BeginSource()
        {
            if (BeginSourceNative())
            {
                ClearData();
                return true;
            }

            return false;
        }

        [NativeMethod("BeginSource")]
        private static partial bool BeginSourceNative();

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

        [NativeProperty]
        public static partial bool IsActive { get; }
    }
}
