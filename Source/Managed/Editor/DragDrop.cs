using March.Core;
using March.Core.Interop;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor
{
    public enum DragDropArea
    {
        Item = 0,
        Window = 1,
    }

    public enum DragDropResult
    {
        Accept = 0,
        Reject = 1,
    }

    public static partial class DragDrop
    {
        private static MarchObject? s_Payload;

        static DragDrop()
        {
            Application.OnTick += () =>
            {
                if (!IsActive)
                {
                    s_Payload = null;
                }
            };
        }

        [NativeMethod]
        public static partial bool BeginSource();

        public static void EndSource(StringLike tooltip, MarchObject payload)
        {
            EndSourceNative(tooltip);
            s_Payload = payload;
        }

        [NativeMethod("EndSource")]
        private static partial void EndSourceNative(StringLike tooltip);

        public static bool BeginTarget(DragDropArea area, [NotNullWhen(true)] out MarchObject? payload, out bool isDelivery)
        {
            if (!BeginTargetNative(area == DragDropArea.Window))
            {
                payload = null;
                isDelivery = false;
                return false;
            }

            if (!CheckPayloadNative(out isDelivery))
            {
                EndTargetNative();

                payload = null;
                return false;
            }

            if ((payload = s_Payload) == null)
            {
                EndTarget(DragDropResult.Reject);
                return false;
            }

            return true;
        }

        public static void EndTarget(DragDropResult result)
        {
            AcceptTargetNative(result == DragDropResult.Accept);
            EndTargetNative();
        }

        [NativeMethod("BeginTarget")]
        private static partial bool BeginTargetNative(bool useWindow);

        [NativeMethod("CheckPayload")]
        private static partial bool CheckPayloadNative(out bool isDelivery);

        [NativeMethod("AcceptTarget")]
        private static partial void AcceptTargetNative(bool accept);

        [NativeMethod("EndTarget")]
        private static partial void EndTargetNative();

        [NativeProperty]
        public static partial bool IsActive { get; }
    }
}
