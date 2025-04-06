using March.Core.Interop;
using March.Core.Pool;
using March.Editor.AssetPipeline;
using System.Text;

namespace March.Editor
{
    public static class EditorGUIUtility
    {
        public static PooledObject<StringBuilder> BuildAssetPath(AssetImporter importer, string? assetGuid = null, bool useCompleteAssetPath = true)
        {
            PooledObject<StringBuilder> builder = StringBuilderPool.Get();
            builder.Value.AppendAssetPath(importer, assetGuid, useCompleteAssetPath);
            return builder;
        }

        public static StringBuilder AppendAssetPath(this StringBuilder builder, AssetImporter importer, string? assetGuid = null, bool useCompleteAssetPath = true)
        {
            string? icon = importer.GetAssetNormalIcon(assetGuid ?? importer.MainAssetGuid);

            if (icon != null)
            {
                builder.Append(icon).Append(' ');
            }

            if (useCompleteAssetPath)
            {
                builder.Append(importer.Location.AssetPath);
            }
            else
            {
                ReadOnlySpan<char> completePath = importer.Location.AssetPath.AsSpan();
                int startIndex = completePath.LastIndexOf('/') + 1;
                builder.Append(completePath[startIndex..]);
            }

            if (assetGuid != null && assetGuid != importer.MainAssetGuid)
            {
                builder.Append(' ').Append('[').Append(importer.GetAssetName(assetGuid)).Append(']');
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

        private static readonly string[] s_SizeUnits = ["B", "KB", "MB", "GB"];

        public static StringBuilder AppendSize(this StringBuilder builder, long sizeInBytes)
        {
            int unit = 0;
            float size = sizeInBytes;

            while (unit < s_SizeUnits.Length - 1 && size > 1024.0f)
            {
                size /= 1024.0f;
                unit++;
            }

            return builder.Append($"{size:F1}").Append(' ').Append(s_SizeUnits[unit]);
        }
    }
}
