using System.Text;

namespace March.Core
{
    public readonly ref struct PooledObject<T>(BaseObjectPool<T> pool) where T : class
    {
        public T Value { get; } = pool.Rent();

        public void Dispose()
        {
            pool.Return(Value);
        }

        public static implicit operator T(PooledObject<T> pooled) => pooled.Value;
    }

    public abstract class BaseObjectPool<T> where T : class
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

    public interface IPoolableObject
    {
        void Reset();
    }

    public sealed class ObjectPool<T> : BaseObjectPool<T> where T : class, IPoolableObject, new()
    {
        protected override T Create() => new();

        protected override void Reset(T value) => value.Reset();
    }

    public sealed class ListPool<T> : BaseObjectPool<List<T>>
    {
        public static ListPool<T> Shared { get; } = new ListPool<T>();

        public static PooledObject<List<T>> Get() => Shared.Use();

        protected override List<T> Create() => [];

        protected override void Reset(List<T> value) => value.Clear();
    }

    public sealed class HashSetPool<T> : BaseObjectPool<HashSet<T>>
    {
        public static HashSetPool<T> Shared { get; } = new HashSetPool<T>();

        public static PooledObject<HashSet<T>> Get() => Shared.Use();

        protected override HashSet<T> Create() => [];

        protected override void Reset(HashSet<T> value) => value.Clear();
    }

    public sealed class DictionaryPool<TKey, TValue> : BaseObjectPool<Dictionary<TKey, TValue>> where TKey : notnull
    {
        public static DictionaryPool<TKey, TValue> Shared { get; } = new DictionaryPool<TKey, TValue>();

        public static PooledObject<Dictionary<TKey, TValue>> Get() => Shared.Use();

        protected override Dictionary<TKey, TValue> Create() => [];

        protected override void Reset(Dictionary<TKey, TValue> value) => value.Clear();
    }

    public sealed class StringBuilderPool : BaseObjectPool<StringBuilder>
    {
        public static StringBuilderPool Shared { get; } = new StringBuilderPool();

        public static PooledObject<StringBuilder> Get() => Shared.Use();

        protected override StringBuilder Create() => new();

        protected override void Reset(StringBuilder value) => value.Clear();
    }
}
