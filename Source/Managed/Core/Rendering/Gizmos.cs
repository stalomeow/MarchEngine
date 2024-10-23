using March.Core.Binding;
using System.Numerics;

namespace March.Core.Rendering
{
    public static partial class Gizmos
    {
        public static void DrawText(Vector3 centerWS, string text)
        {
            DrawText(centerWS, text, Color.White);
        }

        public static void DrawText(Vector3 centerWS, string text, Color color)
        {
            using NativeString t = text;

            Gizmos_DrawText(centerWS, t.Data, color);
        }

        public static void DrawWireArc(Vector3 centerWS, Vector3 normalWS, Vector3 startDirWS, float angleDeg, float radius)
        {
            DrawWireArc(centerWS, normalWS, startDirWS, angleDeg, radius, Color.White);
        }

        public static void DrawWireArc(Vector3 centerWS, Vector3 normalWS, Vector3 startDirWS, float angleDeg, float radius, Color color)
        {
            Gizmos_DrawWireArc(centerWS, normalWS, startDirWS, angleDeg, radius, color);
        }

        public static void DrawWireCircle(Vector3 centerWS, Vector3 normalWS, float radius)
        {
            DrawWireCircle(centerWS, normalWS, radius, Color.White);
        }

        public static void DrawWireCircle(Vector3 centerWS, Vector3 normalWS, float radius, Color color)
        {
            Gizmos_DrawWireCircle(centerWS, normalWS, radius, color);
        }

        public static void DrawWireSphere(Vector3 centerWS, float radius)
        {
            DrawWireSphere(centerWS, radius, Color.White);
        }

        public static void DrawWireSphere(Vector3 centerWS, float radius, Color color)
        {
            Gizmos_DrawWireSphere(centerWS, radius, color);
        }

        public static void DrawLine(Vector3 posWS1, Vector3 posWS2)
        {
            DrawLine(posWS1, posWS2, Color.White);
        }

        public static void DrawLine(Vector3 posWS1, Vector3 posWS2, Color color)
        {
            Gizmos_DrawLine(posWS1, posWS2, color);
        }

        #region Bindings

        [NativeFunction]
        private static partial void Gizmos_DrawText(Vector3 centerWS, nint text, Color color);

        [NativeFunction]
        private static partial void Gizmos_DrawWireArc(Vector3 centerWS, Vector3 normalWS, Vector3 startDirWS, float angleDeg, float radius, Color color);

        [NativeFunction]
        private static partial void Gizmos_DrawWireCircle(Vector3 centerWS, Vector3 normalWS, float radius, Color color);

        [NativeFunction]
        private static partial void Gizmos_DrawWireSphere(Vector3 centerWS, float radius, Color color);

        [NativeFunction]
        private static partial void Gizmos_DrawLine(Vector3 posWS1, Vector3 posWS2, Color color);

        #endregion
    }
}
