namespace March.Core.Pool
{
    public readonly ref struct PooledObject<T>(BaseThreadSafeObjectPool<T> pool) where T : class
    {
        public T Value { get; } = pool.Rent();

        public void Dispose() => pool.Return(Value);

        public static implicit operator T(PooledObject<T> pooled) => pooled.Value;
    }

    public abstract class BaseThreadSafeObjectPool<T> where T : class
    {
        private readonly Stack<T> m_Values = [];
        private readonly Lock m_Lock = new();

        public T Rent()
        {
            using (m_Lock.EnterScope())
            {
                if (m_Values.TryPop(out T? value))
                {
                    return value;
                }
            }

            return Create();
        }

        public void Return(T value)
        {
            Reset(value);

            using (m_Lock.EnterScope())
            {
                m_Values.Push(value);
            }
        }

        public PooledObject<T> Use() => new(this);

        protected abstract T Create();

        protected abstract void Reset(T value);
    }

    public interface IPoolableObject
    {
        void Reset();
    }

    public sealed class ThreadSafeObjectPool<T> : BaseThreadSafeObjectPool<T> where T : class, IPoolableObject, new()
    {
        protected override T Create() => new();

        protected override void Reset(T value) => value.Reset();
    }
}
