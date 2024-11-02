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

    public static unsafe partial class DragDrop
    {
        private static MarchObject? s_Payload;

        public static void EndSource(StringLike tooltip, MarchObject payload)
        {
            using NativeString t = tooltip;
            DragDrop_EndSource(t.Data);

            s_Payload = payload;
        }

        public static bool BeginTarget(DragDropArea area, [NotNullWhen(true)] out MarchObject? payload, out bool isDelivery)
        {
            if (!DragDrop_BeginTarget(area == DragDropArea.Window))
            {
                payload = null;
                isDelivery = false;
                return false;
            }

            fixed (bool* pIsDelivery = &isDelivery)
            {
                if (!DragDrop_CheckPayload(pIsDelivery))
                {
                    DragDrop_EndTarget();

                    payload = null;
                    return false;
                }
            }

            if ((payload = s_Payload) == null)
            {
                EndTarget(accept: false);
                return false;
            }

            return true;
        }

        public static void EndTarget(bool accept)
        {
            DragDrop_AcceptTarget(accept);
            DragDrop_EndTarget();
        }

        public static bool IsActive => DragDrop_IsActive();

        internal static void Update()
        {
            if (!IsActive)
            {
                s_Payload = null;
            }
        }

        #region Bindings

        [NativeFunction(Name = "DragDrop_BeginSource")]
        public static partial bool BeginSource();

        [NativeFunction]
        private static partial void DragDrop_EndSource(nint tooltip);

        [NativeFunction]
        private static partial bool DragDrop_BeginTarget(bool useWindow);

        [NativeFunction]
        private static partial bool DragDrop_CheckPayload(bool* outIsDelivery);

        [NativeFunction]
        private static partial void DragDrop_AcceptTarget(bool accept);

        [NativeFunction]
        private static partial void DragDrop_EndTarget();

        [NativeFunction]
        private static partial bool DragDrop_IsActive();

        #endregion
    }
}
