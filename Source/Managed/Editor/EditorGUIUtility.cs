using March.Core;
using System.Text;

namespace March.Editor
{
    public static class EditorGUIUtility
    {
        public static PooledObject<StringBuilder> BuildIconText(string icon, string text, string? id = null)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();
            builder.Value.Append(icon).Append(' ').Append(text);
            builder.Value.AppendId(id ?? text); // 使得 ImGui Id 与图标无关，切换图标后状态不会丢失
            return builder;
        }

        public static PooledObject<StringBuilder> BuildId(string id)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();
            builder.Value.AppendId(id);
            return builder;
        }

        public static StringBuilder AppendId(this StringBuilder builder, string id)
        {
            // https://github.com/ocornut/imgui/blob/master/docs/FAQ.md#q-about-the-id-stack-system
            return builder.Append("###").Append(id);
        }
    }
}
