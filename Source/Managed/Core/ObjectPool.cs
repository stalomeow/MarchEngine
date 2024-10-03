namespace March.Core
{
    public readonly ref struct PooledObject<T>(ObjectPool<T> pool) where T : class
    {
        public T Value { get; } = pool.Rent();

        public void Dispose()
        {
            pool.Return(Value);
        }

        public static implicit operator T(PooledObject<T> pooled) => pooled.Value;
    }

    public abstract class ObjectPool<T> where T : class
    {
        private readonly Stack<T> m_Values = [];

        public T Rent()
        {
            if (!m_Values.TryPop(out T? value))
            {
                value = Create();
            }

            return value;
        }

        public void Return(T value)
        {
            Reset(value);
            m_Values.Push(value);
        }

        public PooledObject<T> Use() => new(this);

        protected abstract T Create();

        protected abstract void Reset(T value);
    }

    public sealed class ListPool<T> : ObjectPool<List<T>>
    {
        public static ListPool<T> Shared { get; } = new ListPool<T>();

        public static PooledObject<List<T>> Get() => Shared.Use();

        protected override List<T> Create() => [];

        protected override void Reset(List<T> value) => value.Clear();
    }

    public sealed class HashSetPool<T> : ObjectPool<HashSet<T>>
    {
        public static HashSetPool<T> Shared { get; } = new HashSetPool<T>();

        public static PooledObject<HashSet<T>> Get() => Shared.Use();

        protected override HashSet<T> Create() => [];

        protected override void Reset(HashSet<T> value) => value.Clear();
    }

    public sealed class DictionaryPool<TKey, TValue> : ObjectPool<Dictionary<TKey, TValue>> where TKey : notnull
    {
        public static DictionaryPool<TKey, TValue> Shared { get; } = new DictionaryPool<TKey, TValue>();

        public static PooledObject<Dictionary<TKey, TValue>> Get() => Shared.Use();

        protected override Dictionary<TKey, TValue> Create() => [];

        protected override void Reset(Dictionary<TKey, TValue> value) => value.Clear();
    }
}
