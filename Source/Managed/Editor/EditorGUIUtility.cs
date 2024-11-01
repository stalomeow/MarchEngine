using March.Core.Interop;
using March.Core.Pool;
using System.Text;

namespace March.Editor
{
    public static class EditorGUIUtility
    {
        public static PooledObject<StringBuilder> BuildIconText(StringLike icon, StringLike text)
        {
            return BuildIconText(icon, text, text); // 使得 ImGui Id 与图标无关，切换图标后状态不会丢失
        }

        public static PooledObject<StringBuilder> BuildIconText(StringLike icon, StringLike text, StringLike id)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();
            builder.Value.Append(icon).Append(' ').Append(text).AppendId(id);
            return builder;
        }

        public static PooledObject<StringBuilder> BuildId(StringLike id)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();
            builder.Value.AppendId(id);
            return builder;
        }

        public static StringBuilder AppendId(this StringBuilder builder, StringLike id)
        {
            // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-about-the-id-stack-system
            return builder.Append("###").Append(id);
        }
    }
}
