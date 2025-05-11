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
        /// 接受并显示一个矩形框
        /// </summary>
        AcceptByRect,

        /// <summary>
        /// 接受并显示一条线
        /// </summary>
        AcceptByLine,

        /// <summary>
        /// 拒绝
        /// </summary>
        Reject,
    }

    public readonly ref struct DragDropData
    {
        public required object Payload { get; init; }

        /// <summary>
        /// 可用于判断 <see cref="Payload"/> 的来源
        /// </summary>
        public required object? Context { get; init; }

        /// <summary>
        /// 用户释放鼠标按键时，为 <see langword="true"/>
        /// </summary>
        public required bool IsDelivery { get; init; }
    }

    public static partial class DragDrop
    {
        private static object? s_Payload;
        private static object? s_Context;

        static DragDrop()
        {
            Application.OnTick += () =>
            {
                if (!IsActive)
                {
                    s_Payload = null;
                    s_Context = null;
                }
            };
        }

        [NativeMethod]
        public static partial bool BeginSource();

        public static void EndSource(StringLike tooltip, object payload, object? context = null)
        {
            EndSourceNative(tooltip);
            s_Payload = payload;
            s_Context = context;
        }

        [NativeMethod("EndSource")]
        private static partial void EndSourceNative(StringLike tooltip);

        public static bool BeginTarget(DragDropArea area, out DragDropData data)
        {
            if (!BeginTargetNative(area == DragDropArea.Window))
            {
                data = default;
                return false;
            }

            if (!CheckPayloadNative(out bool isDelivery))
            {
                EndTargetNative();

                data = default;
                return false;
            }

            if (s_Payload == null)
            {
                EndTarget(DragDropResult.Reject);

                data = default;
                return false;
            }

            data = new DragDropData
            {
                Payload = s_Payload,
                Context = s_Context,
                IsDelivery = isDelivery
            };
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
