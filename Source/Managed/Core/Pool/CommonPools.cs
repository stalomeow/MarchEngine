using System.Text;

namespace March.Core.Pool
{
    public sealed class ListPool<T> : BaseThreadSafeObjectPool<List<T>>
    {
        public static ListPool<T> Shared { get; } = new ListPool<T>();

        public static PooledObject<List<T>> Get() => Shared.Use();

        protected override List<T> Create() => [];

        protected override void Reset(List<T> value) => value.Clear();
    }

    public sealed class HashSetPool<T> : BaseThreadSafeObjectPool<HashSet<T>>
    {
        public static HashSetPool<T> Shared { get; } = new HashSetPool<T>();

        public static PooledObject<HashSet<T>> Get() => Shared.Use();

        protected override HashSet<T> Create() => [];

        protected override void Reset(HashSet<T> value) => value.Clear();
    }

    public sealed class DictionaryPool<TKey, TValue> : BaseThreadSafeObjectPool<Dictionary<TKey, TValue>> where TKey : notnull
    {
        public static DictionaryPool<TKey, TValue> Shared { get; } = new DictionaryPool<TKey, TValue>();

        public static PooledObject<Dictionary<TKey, TValue>> Get() => Shared.Use();

        protected override Dictionary<TKey, TValue> Create() => [];

        protected override void Reset(Dictionary<TKey, TValue> value) => value.Clear();
    }

    public sealed class StringBuilderPool : BaseThreadSafeObjectPool<StringBuilder>
    {
        public static StringBuilderPool Shared { get; } = new StringBuilderPool();

        public static PooledObject<StringBuilder> Get() => Shared.Use();

        protected override StringBuilder Create() => new();

        protected override void Reset(StringBuilder value) => value.Clear();
    }
}
