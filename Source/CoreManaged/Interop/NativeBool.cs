using System.Runtime.InteropServices;

namespace March.Core.Interop
{
    [StructLayout(LayoutKind.Sequential)]
    public struct NativeBool
    {
        public int Data;

        public static implicit operator NativeBool(bool v) => new() { Data = v ? 1 : 0 };
        public static implicit operator bool(NativeBool v) => v.Data != 0;
    }
}
