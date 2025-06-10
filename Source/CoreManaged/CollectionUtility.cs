using System.Numerics;

namespace March.Core
{
    public static class CollectionUtility
    {
        public static T Sum<T>(this ReadOnlySpan<T> span) where T : IAdditiveIdentity<T, T>, IAdditionOperators<T, T, T>
        {
            T result = T.AdditiveIdentity;

            foreach (T value in span)
            {
                result += value;
            }

            return result;
        }

        public static T Sum<T>(this Span<T> span) where T : IAdditiveIdentity<T, T>, IAdditionOperators<T, T, T>
        {
            return Sum((ReadOnlySpan<T>)span);
        }
    }
}
