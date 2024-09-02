using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace DX12Demo.Core.Binding
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe partial struct NativeArray<T> : IDisposable where T : unmanaged
    {
        public nint Data;

        public NativeArray(int length)
        {
            Data = NewArray(length * sizeof(T));
        }

        public void Dispose()
        {
            Free(Data);
            Data = nint.Zero;
        }

        public T[] GetAndDispose()
        {
            T[] result = Get(Data);
            Dispose();
            return result;
        }

        public readonly T[] Value => Get(Data);

        public readonly int Length
        {
            get
            {
                byte* pData = null;
                int byteCount = 0;
                UnmarshalArray(Data, &pData, &byteCount);
                return byteCount / sizeof(T);
            }
        }

        public readonly ref T this[int index]
        {
            get
            {
                byte* pData = null;
                int byteCount = 0;
                UnmarshalArray(Data, &pData, &byteCount);

                int byteIndex = index * sizeof(T);
                if (byteIndex < 0 || byteIndex >= byteCount)
                {
                    throw new IndexOutOfRangeException();
                }

                return ref Unsafe.AsRef<T>(pData + byteIndex);
            }
        }

        public static implicit operator NativeArray<T>(T[] value) => new() { Data = New(value) };

        public static explicit operator NativeArray<T>(nint data) => new() { Data = data };

        public static nint New(T[] array)
        {
            fixed (T* p = array)
            {
                return MarshalArray((byte*)p, array.Length * sizeof(T));
            }
        }

        public static T[] Get(nint data)
        {
            byte* pData = null;
            int byteCount = 0;
            UnmarshalArray(data, &pData, &byteCount);

            if (byteCount == 0)
            {
                return Array.Empty<T>();
            }

            var result = new T[byteCount / sizeof(T)];
            fixed (T* p = result)
            {
                Buffer.MemoryCopy(pData, p, byteCount, byteCount);
            }
            return result;
        }

        public static T[] GetAndFree(nint data)
        {
            T[] result = Get(data);
            Free(data);
            return result;
        }

        [NativeFunction]
        private static partial nint NewArray(int byteCount);

        [NativeFunction]
        private static partial nint MarshalArray(byte* p, int byteCount);

        [NativeFunction]
        private static partial void UnmarshalArray(nint self, byte** ppOutData, int* pOutByteCount);

        [NativeFunction(Name = "FreeArray")]
        public static partial void Free(nint data);
    }

    public interface INativeMarshal<T> where T : INativeMarshal<T>
    {
        static abstract T FromNative(nint data);

        static abstract void ToNative(T value, nint data);

        static abstract void FreeNative(nint data);

        static abstract int SizeOfNative();
    }

    public interface INativeMarshal<TManaged, TNative> : INativeMarshal<TManaged>
        where TManaged : INativeMarshal<TManaged, TNative>
        where TNative : unmanaged
    {
        static unsafe TManaged INativeMarshal<TManaged>.FromNative(nint data)
        {
            return TManaged.FromNative(ref Unsafe.AsRef<TNative>((void*)data));
        }

        static unsafe void INativeMarshal<TManaged>.ToNative(TManaged value, nint data)
        {
            TManaged.ToNative(value, out Unsafe.AsRef<TNative>((void*)data));
        }

        static unsafe void INativeMarshal<TManaged>.FreeNative(nint data)
        {
            TManaged.FreeNative(ref Unsafe.AsRef<TNative>((void*)data));
        }

        static abstract TManaged FromNative(ref TNative native);

        static abstract void ToNative(TManaged value, out TNative native);

        static abstract void FreeNative(ref TNative native);
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct NativeArrayMarshal<T> : IDisposable where T : INativeMarshal<T>
    {
        public NativeArray<byte> Array;

        public NativeArrayMarshal(int length)
        {
            Array = new NativeArray<byte>(T.SizeOfNative() * length);
        }

        public NativeArrayMarshal(T[] array) : this(array.Length)
        {
            for (int i = 0; i < array.Length; i++)
            {
                T.ToNative(array[i], (nint)Unsafe.AsPointer(ref Array[i * T.SizeOfNative()]));
            }
        }

        public void Dispose()
        {
            int nativeSize = T.SizeOfNative();
            int elementCount = Array.Length / nativeSize;

            for (int i = 0; i < elementCount; i++)
            {
                T.FreeNative((nint)Unsafe.AsPointer(ref Array[i * nativeSize]));
            }

            Array.Dispose();
        }

        public T[] GetAndDispose()
        {
            T[] result = Value;
            Dispose();
            return result;
        }

        public readonly T[] Value
        {
            get
            {
                int nativeSize = T.SizeOfNative();
                T[] result = new T[Array.Length / nativeSize];

                for (int i = 0; i < result.Length; i++)
                {
                    result[i] = T.FromNative((nint)Unsafe.AsPointer(ref Array[i * nativeSize]));
                }

                return result;
            }
        }

        public static implicit operator NativeArrayMarshal<T>(T[] value) => new(value);
    }
}
