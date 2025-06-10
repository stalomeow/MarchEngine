using March.Core.Pool;
using System.Text;

namespace March.Core.Interop
{
    public readonly ref struct StringLike
    {
        /// <summary>
        /// 可能是 <see cref="string"/> 或 <see cref="StringBuilder"/>
        /// </summary>
        public object Value { get; }

        public StringLike(string value) => Value = value;

        public StringLike(StringBuilder value) => Value = value;

        public int Length => Value switch
        {
            string s => s.Length,
            StringBuilder sb => sb.Length,
            _ => throw new ArgumentException("Invalid string type"),
        };

        public char this[int index] => Value switch
        {
            string s => s[index],
            StringBuilder sb => sb[index],
            _ => throw new ArgumentException("Invalid string type"),
        };

        public override string ToString() => Value switch
        {
            string s => s,
            StringBuilder sb => sb.ToString(),
            _ => throw new ArgumentException("Invalid string type"),
        };

        public static implicit operator StringLike(string value) => new(value);

        public static implicit operator StringLike(StringBuilder value) => new(value);

        public static implicit operator StringLike(PooledObject<StringBuilder> value) => new(value);
    }

    public static class StringLikeUtility
    {
        public static StringBuilder Append(this StringBuilder builder, StringLike value) => value.Value switch
        {
            string s => builder.Append(s),
            StringBuilder sb => builder.Append(sb),
            _ => throw new ArgumentException("Invalid string type", nameof(value)),
        };
    }
}
