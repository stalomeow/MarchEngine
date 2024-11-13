using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace March.Core.Interop
{
    [StructLayout(LayoutKind.Sequential)]
    public unsafe partial struct NativeArray<T> : IDisposable where T : unmanaged
    {
        public nint Data;

        public NativeArray(int length)
        {
            Data = NativeArrayBindings.NewArray(length * sizeof(T));
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
                NativeArrayBindings.UnmarshalArray(Data, &pData, &byteCount);
                return byteCount / sizeof(T);
            }
        }

        public readonly ref T this[int index]
        {
            get
            {
                byte* pData = null;
                int byteCount = 0;
                NativeArrayBindings.UnmarshalArray(Data, &pData, &byteCount);

                int byteIndex = index * sizeof(T);
                if (byteIndex < 0 || byteIndex >= byteCount)
                {
                    throw new IndexOutOfRangeException();
                }

                return ref Unsafe.AsRef<T>(pData + byteIndex);
            }
        }

        public delegate U ElementConverter<U>(in T element);

        public readonly U[] ConvertValue<U>(ElementConverter<U> converter)
        {
            U[] results = new U[Length];

            for (int i = 0; i < results.Length; i++)
            {
                results[i] = converter(in this[i]);
            }

            return results;
        }

        public static implicit operator NativeArray<T>(ReadOnlySpan<T> value) => new() { Data = New(value) };

        public static implicit operator NativeArray<T>(T[] value) => new() { Data = New(value) };

        public static implicit operator NativeArray<T>(List<T> value) => new() { Data = New(value) };

        public static explicit operator NativeArray<T>(nint data) => new() { Data = data };

        public static nint New(ReadOnlySpan<T> span)
        {
            fixed (T* p = span)
            {
                return NativeArrayBindings.MarshalArray((byte*)p, span.Length * sizeof(T));
            }
        }

        public static nint New(T[] array)
        {
            return New(array.AsSpan());
        }

        public static nint New(List<T> list)
        {
            return New(CollectionsMarshal.AsSpan(list));
        }

        public static T[] Get(nint data)
        {
            byte* pData = null;
            int byteCount = 0;
            NativeArrayBindings.UnmarshalArray(data, &pData, &byteCount);

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

        public static void Free(nint data) => NativeArrayBindings.FreeArray(data);
    }

    internal static unsafe partial class NativeArrayBindings
    {
        [NativeMethod]
        public static partial nint NewArray(int byteCount);

        [NativeMethod]
        public static partial nint MarshalArray(byte* p, int byteCount);

        [NativeMethod]
        public static partial void UnmarshalArray(nint self, byte** ppOutData, int* pOutByteCount);

        [NativeMethod]
        public static partial void FreeArray(nint data);
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

        static unsafe int INativeMarshal<TManaged>.SizeOfNative()
        {
            return sizeof(TNative);
        }

        static abstract TManaged FromNative(ref TNative native);

        static abstract void ToNative(TManaged value, out TNative native);

        static abstract void FreeNative(ref TNative native);
    }

    [StructLayout(LayoutKind.Sequential)]
    public unsafe struct NativeArrayMarshal<T> : IDisposable where T : INativeMarshal<T>
    {
        public nint Data;

        public NativeArrayMarshal(int length)
        {
            Data = NativeArrayBindings.NewArray(length * T.SizeOfNative());
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

        public static implicit operator NativeArrayMarshal<T>(T[] value) => new() { Data = New(value) };

        public static explicit operator NativeArrayMarshal<T>(nint data) => new() { Data = data };

        public static nint New(T[] array)
        {
            var data = new NativeArray<byte>(T.SizeOfNative() * array.Length);

            for (int i = 0; i < array.Length; i++)
            {
                T.ToNative(array[i], (nint)Unsafe.AsPointer(ref data[i * T.SizeOfNative()]));
            }

            return data.Data;
        }

        public static T[] Get(nint data)
        {
            var array = (NativeArray<byte>)data;
            int nativeSize = T.SizeOfNative();
            T[] result = new T[array.Length / nativeSize];

            for (int i = 0; i < result.Length; i++)
            {
                result[i] = T.FromNative((nint)Unsafe.AsPointer(ref array[i * nativeSize]));
            }

            return result;
        }

        public static T[] GetAndFree(nint data)
        {
            T[] result = Get(data);
            Free(data);
            return result;
        }

        public static void Free(nint data)
        {
            var array = (NativeArray<byte>)data;
            int nativeSize = T.SizeOfNative();
            int elementCount = array.Length / nativeSize;

            for (int i = 0; i < elementCount; i++)
            {
                T.FreeNative((nint)Unsafe.AsPointer(ref array[i * nativeSize]));
            }

            array.Dispose();
        }
    }
}
