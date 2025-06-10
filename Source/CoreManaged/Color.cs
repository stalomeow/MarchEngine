using System.Runtime.InteropServices;

namespace March.Core
{
    [StructLayout(LayoutKind.Sequential)]
    public struct Color(float r, float g, float b, float a)
    {
        public float R = r;
        public float G = g;
        public float B = b;
        public float A = a;

        public static Color White => new(1, 1, 1, 1);

        public static Color Black => new(0, 0, 0, 1);

        public static Color Red => new(1, 0, 0, 1);

        public static Color Green => new(0, 1, 0, 1);

        public static Color Blue => new(0, 0, 1, 1);

        public static Color Yellow => new(1, 1, 0, 1);

        public static Color Cyan => new(0, 1, 1, 1);

        public static Color Magenta => new(1, 0, 1, 1);

        public static Color Transparent => new(0, 0, 0, 0);
    }
}
