using System.Numerics;
using System.Runtime.InteropServices;

namespace March.Core
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Bounds(Vector3 center, Vector3 extents)
    {
        public Vector3 Center = center;
        public Vector3 Extents = extents;

        public readonly Vector3 Size => Extents * 2.0f;
    }
}
