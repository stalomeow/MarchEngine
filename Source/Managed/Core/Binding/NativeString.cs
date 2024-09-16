using System.Runtime.InteropServices;

namespace March.Core.Binding
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe partial struct NativeString : IDisposable
    {
        public nint Data;

        public void Dispose()
        {
            Free(Data);
            Data = nint.Zero;
        }

        public readonly string Value => Get(Data);

        public static implicit operator NativeString(string value) => new() { Data = New(value) };

        public static explicit operator NativeString(nint data) => new() { Data = data };

        public static nint New(string s)
        {
            fixed (char* p = s)
            {
                return MarshalString(p, s.Length);
            }
        }

        public static string Get(nint s)
        {
            char* pData;
            int len;
            UnmarshalString(s, &pData, &len);
            return len == 0 ? string.Empty : new string(pData, 0, len);
        }

        public static string GetAndFree(nint s)
        {
            string result = Get(s);
            Free(s);
            return result;
        }

        [NativeFunction]
        private static partial nint MarshalString(char* p, int len);

        [NativeFunction]
        private static partial void UnmarshalString(nint s, char** ppOutData, int* pOutLen);

        [NativeFunction(Name = "FreeString")]
        public static partial void Free(nint s);
    }
}
