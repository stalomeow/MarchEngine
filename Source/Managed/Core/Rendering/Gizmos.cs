using March.Core.Interop;
using System.Numerics;

namespace March.Core.Rendering
{
    public static partial class Gizmos
    {
        public static bool IsGUIMode => Gizmos_IsGUIMode();

        /// <summary>
        /// 原点处的 GUI 缩放比例
        /// </summary>
        public static float GUIScaleAtOrigin => GetGUIScale(Vector3.Zero);

        public static void DrawText(Vector3 center, string text)
        {
            using NativeString t = text;

            Gizmos_DrawText(center, t.Data);
        }

        public readonly ref struct MatrixScope
        {
            [Obsolete("Use other constructors", error: true)]
            public MatrixScope() { }

            public MatrixScope(Matrix4x4 matrix)
            {
                Gizmos_PushMatrix(matrix);
            }

            public void Dispose()
            {
                Gizmos_PopMatrix();
            }
        }

        public readonly ref struct ColorScope
        {
            [Obsolete("Use other constructors", error: true)]
            public ColorScope() { }

            public ColorScope(Color color)
            {
                Gizmos_PushColor(color);
            }

            public void Dispose()
            {
                Gizmos_PopColor();
            }
        }

        #region Bindings

        [NativeMethod]
        private static partial bool Gizmos_IsGUIMode();

        [NativeMethod(Name = "Gizmos_Clear")]
        internal static partial void Clear();

        [NativeMethod]
        private static partial void Gizmos_PushMatrix(Matrix4x4 matrix);

        [NativeMethod]
        private static partial void Gizmos_PopMatrix();

        [NativeMethod]
        private static partial void Gizmos_PushColor(Color color);

        [NativeMethod]
        private static partial void Gizmos_PopColor();

        [NativeMethod(Name = "Gizmos_GetGUIScale")]
        public static partial float GetGUIScale(Vector3 position);

        [NativeMethod(Name = "Gizmos_DrawLine")]
        public static partial void DrawLine(Vector3 p1, Vector3 p2);

        [NativeMethod(Name = "Gizmos_DrawWireArc")]
        public static partial void DrawWireArc(Vector3 center, Vector3 normal, Vector3 startDir, float radians, float radius);

        [NativeMethod(Name = "Gizmos_DrawWireDisc")]
        public static partial void DrawWireDisc(Vector3 center, Vector3 normal, float radius);

        [NativeMethod(Name = "Gizmos_DrawWireSphere")]
        public static partial void DrawWireSphere(Vector3 center, float radius);

        [NativeMethod(Name = "Gizmos_DrawWireCube")]
        public static partial void DrawWireCube(Vector3 center, Vector3 size);

        [NativeMethod]
        private static partial void Gizmos_DrawText(Vector3 center, nint text);

        #endregion
    }

    public static class GizmosUtility
    {
        public static Matrix4x4 GetLocalToWorldMatrixWithoutScale(Transform transform)
        {
            return Matrix4x4.CreateFromQuaternion(transform.Rotation) * Matrix4x4.CreateTranslation(transform.Position);
        }
    }
}
