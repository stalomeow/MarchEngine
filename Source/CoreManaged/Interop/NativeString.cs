using March.Core.Pool;
using System.Runtime.InteropServices;
using System.Text;

namespace March.Core.Interop
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

        public static implicit operator NativeString(Span<char> value) => new() { Data = New(value) };

        public static implicit operator NativeString(ReadOnlySpan<char> value) => new() { Data = New(value) };

        public static implicit operator NativeString(StringBuilder value) => new() { Data = New(value) };

        public static implicit operator NativeString(PooledObject<StringBuilder> value) => new() { Data = New(value) };

        public static implicit operator NativeString(StringLike value) => new() { Data = New(value) };

        public static explicit operator NativeString(nint data) => new() { Data = data };

        public static nint New(string s)
        {
            return New(s.AsSpan());
        }

        public static nint New(Span<char> s)
        {
            return New((ReadOnlySpan<char>)s);
        }

        public static nint New(ReadOnlySpan<char> s)
        {
            fixed (char* p = s)
            {
                return Marshal(p, s.Length);
            }
        }

        public static nint New(StringBuilder builder)
        {
            nint result = New(builder.Length);
            int offset = 0;

            foreach (ReadOnlyMemory<char> chunk in builder.GetChunks())
            {
                if (chunk.IsEmpty)
                {
                    continue;
                }

                fixed (char* p = chunk.Span)
                {
                    SetData(result, offset, p, chunk.Length);
                }

                offset += chunk.Length;
            }

            return result;
        }

        public static nint New(StringLike value)
        {
            if (value.Value is string s)
            {
                return New(s);
            }

            if (value.Value is StringBuilder builder)
            {
                return New(builder);
            }

            throw new ArgumentException("Invalid string type", nameof(value));
        }

        public static string Get(nint s)
        {
            Unmarshal(s, out byte* pData, out int len);
            return len == 0 ? string.Empty : Encoding.UTF8.GetString(pData, len);
        }

        public static string GetAndFree(nint s)
        {
            string result = Get(s);
            Free(s);
            return result;
        }

        [NativeMethod]
        private static partial nint Marshal(char* p, int len);

        [NativeMethod]
        private static partial void Unmarshal(nint s, out byte* data, out int len);

        [NativeMethod]
        private static partial void SetData(nint s, int offset, char* p, int count);

        [NativeMethod]
        public static partial nint New(int length);

        [NativeMethod]
        public static partial void Free(nint s);
    }
}
