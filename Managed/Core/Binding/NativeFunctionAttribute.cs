using System.ComponentModel;
using System.Runtime.InteropServices;

namespace DX12Demo.Core.Binding
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class NativeFunctionAttribute : Attribute
    {
        public string? Name { get; set; }

        //[EditorBrowsable(EditorBrowsableState.Never)]
        public static unsafe delegate* unmanaged[Stdcall]<char*, int, nint> LookUpFn;

        [UnmanagedCallersOnly]
        [EditorBrowsable(EditorBrowsableState.Never)]
        public static unsafe void SetLookUpFn(nint fn)
        {
            LookUpFn = (delegate* unmanaged[Stdcall]<char*, int, nint>)fn;
        }
    }
}
