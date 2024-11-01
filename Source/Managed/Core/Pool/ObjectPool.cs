namespace March.Core.Pool
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
}
