using March.Core;
using March.Core.Interop;
using System.Numerics;

namespace March.Editor
{
    public static partial class Gizmos
    {
        [NativeProperty]
        public static partial bool IsGUIMode { get; }

        /// <summary>
        /// 原点处的 GUI 缩放比例
        /// </summary>
        public static float GUIScaleAtOrigin => GetGUIScale(Vector3.Zero);

        public readonly ref struct MatrixScope
        {
            [Obsolete("Use other constructors", error: true)]
            public MatrixScope() { }

            public MatrixScope(Matrix4x4 matrix)
            {
                PushMatrix(matrix);
            }

            public void Dispose()
            {
                PopMatrix();
            }
        }

        public readonly ref struct ColorScope
        {
            [Obsolete("Use other constructors", error: true)]
            public ColorScope() { }

            public ColorScope(Color color)
            {
                PushColor(color);
            }

            public void Dispose()
            {
                PopColor();
            }
        }

        [NativeMethod]
        internal static partial void Clear();

        [NativeMethod]
        private static partial void PushMatrix(Matrix4x4 matrix);

        [NativeMethod]
        private static partial void PopMatrix();

        [NativeMethod]
        private static partial void PushColor(Color color);

        [NativeMethod]
        private static partial void PopColor();

        [NativeMethod]
        public static partial float GetGUIScale(Vector3 position);

        [NativeMethod]
        public static partial void DrawLine(Vector3 p1, Vector3 p2);

        [NativeMethod]
        public static partial void DrawWireArc(Vector3 center, Vector3 normal, Vector3 startDir, float radians, float radius);

        [NativeMethod]
        public static partial void DrawWireDisc(Vector3 center, Vector3 normal, float radius);

        [NativeMethod]
        public static partial void DrawWireSphere(Vector3 center, float radius);

        [NativeMethod]
        public static partial void DrawWireCube(Vector3 center, Vector3 size);

        [NativeMethod]
        public static partial void DrawText(Vector3 center, StringLike text);

        internal static void Initialize()
        {
            InitResources();
            Application.OnQuit += ReleaseResources;
        }

        [NativeMethod]
        private static partial void InitResources();

        [NativeMethod]
        private static partial void ReleaseResources();
    }

    public static class GizmosUtility
    {
        public static Matrix4x4 GetLocalToWorldMatrixWithoutScale(Transform transform)
        {
            return Matrix4x4.CreateFromQuaternion(transform.Rotation) * Matrix4x4.CreateTranslation(transform.Position);
        }
    }
}
