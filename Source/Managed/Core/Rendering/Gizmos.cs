using March.Core.Binding;
using System.Numerics;

namespace March.Core.Rendering
{
    public static partial class Gizmos
    {
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

        [NativeFunction(Name = "Gizmos_Clear")]
        internal static partial void Clear();

        [NativeFunction]
        private static partial void Gizmos_PushMatrix(Matrix4x4 matrix);

        [NativeFunction]
        private static partial void Gizmos_PopMatrix();

        [NativeFunction]
        private static partial void Gizmos_PushColor(Color color);

        [NativeFunction]
        private static partial void Gizmos_PopColor();

        [NativeFunction(Name = "Gizmos_AddLine")]
        public static partial void AddLine(Vector3 p1, Vector3 p2);

        [NativeFunction(Name = "Gizmos_FlushAndDrawLines")]
        public static partial void FlushAndDrawLines();

        [NativeFunction(Name = "Gizmos_DrawWireArc")]
        public static partial void DrawWireArc(Vector3 center, Vector3 normal, Vector3 startDir, float radians, float radius);

        [NativeFunction(Name = "Gizmos_DrawWireCircle")]
        public static partial void DrawWireCircle(Vector3 center, Vector3 normal, float radius);

        [NativeFunction(Name = "Gizmos_DrawWireSphere")]
        public static partial void DrawWireSphere(Vector3 center, float radius);

        [NativeFunction(Name = "Gizmos_DrawWireCube")]
        public static partial void DrawWireCube(Vector3 center, Vector3 size);

        #endregion
    }

    public static partial class GizmosGUI
    {
        public static void DrawText(Vector3 center, string text)
        {
            using NativeString t = text;

            GizmosGUI_DrawText(center, t.Data);
        }

        public readonly ref struct MatrixScope
        {
            [Obsolete("Use other constructors", error: true)]
            public MatrixScope() { }

            public MatrixScope(Matrix4x4 matrix)
            {
                GizmosGUI_PushMatrix(matrix);
            }

            public void Dispose()
            {
                GizmosGUI_PopMatrix();
            }
        }

        public readonly ref struct ColorScope
        {
            [Obsolete("Use other constructors", error: true)]
            public ColorScope() { }

            public ColorScope(Color color)
            {
                GizmosGUI_PushColor(color);
            }

            public void Dispose()
            {
                GizmosGUI_PopColor();
            }
        }

        #region Bindings

        [NativeFunction]
        private static partial void GizmosGUI_PushMatrix(Matrix4x4 matrix);

        [NativeFunction]
        private static partial void GizmosGUI_PopMatrix();

        [NativeFunction]
        private static partial void GizmosGUI_PushColor(Color color);

        [NativeFunction]
        private static partial void GizmosGUI_PopColor();

        [NativeFunction]
        private static partial void GizmosGUI_DrawText(Vector3 center, nint text);

        #endregion
    }
}
