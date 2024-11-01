using March.Core.Pool;
using System.Text;

namespace March.Editor
{
    internal static class GuidUtility
    {
        public static string GetNew()
        {
            // https://learn.microsoft.com/en-us/dotnet/api/system.guid.tostring?view=net-8.0
            return Guid.NewGuid().ToString("N");
        }

        public static PooledObject<StringBuilder> BuildNew()
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();

            var value = Guid.NewGuid();
            Span<char> buffer = stackalloc char[32];

            if (value.TryFormat(buffer, out int charsWritten, "N"))
            {
                builder.Value.Append(buffer[..charsWritten]);
            }
            else
            {
                builder.Value.Append(value.ToString("N"));
            }

            return builder;
        }
    }
}
