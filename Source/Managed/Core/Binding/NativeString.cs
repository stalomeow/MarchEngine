using System.Runtime.InteropServices;
using System.Text;

namespace March.Core.Binding
{
    public readonly ref struct StringView(object data)
    {
        public object Data { get; } = data;

        public static implicit operator StringView(string value) => new(value);

        public static implicit operator StringView(StringBuilder value) => new(value);

        public static implicit operator StringView(PooledObject<StringBuilder> value) => new(value.Value);
    }

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

        public static implicit operator NativeString(StringBuilder value) => new() { Data = New(value) };

        public static implicit operator NativeString(StringView value) => new() { Data = New(value) };

        public static explicit operator NativeString(nint data) => new() { Data = data };

        public static nint New(string s)
        {
            fixed (char* p = s)
            {
                return MarshalString(p, s.Length);
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
                    SetStringData(result, offset, p, chunk.Length);
                }

                offset += chunk.Length;
            }

            return result;
        }

        public static nint New(StringView view)
        {
            if (view.Data is string s)
            {
                return New(s);
            }

            if (view.Data is StringBuilder builder)
            {
                return New(builder);
            }

            throw new NotSupportedException($"Unsupported {nameof(StringView)}");
        }

        public static string Get(nint s)
        {
            byte* pData;
            int len;
            UnmarshalString(s, &pData, &len);
            return len == 0 ? string.Empty : Encoding.UTF8.GetString(pData, len);
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
        private static partial void UnmarshalString(nint s, byte** ppOutData, int* pOutLen);

        [NativeFunction]
        private static partial void SetStringData(nint s, int offset, char* p, int count);

        [NativeFunction(Name = "NewString")]
        public static partial nint New(int length);

        [NativeFunction(Name = "FreeString")]
        public static partial void Free(nint s);
    }
}
