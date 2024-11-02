using March.Core.Interop;
using March.Core.Pool;
using March.Editor.AssetPipeline;
using System.Text;

namespace March.Editor
{
    public static class EditorGUIUtility
    {
        public static PooledObject<StringBuilder> BuildAssetPath(AssetImporter importer, string? assetGuid = null)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();
            builder.Value.AppendAssetPath(importer, assetGuid);
            return builder;
        }

        public static StringBuilder AppendAssetPath(this StringBuilder builder, AssetImporter importer, string? assetGuid = null)
        {
            builder.Append(importer.Location.AssetPath);

            if (assetGuid != null && assetGuid != importer.MainAssetGuid)
            {
                builder.Append(" [").Append(importer.GetAssetName(assetGuid)).Append(']');
            }

            return builder;
        }

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
