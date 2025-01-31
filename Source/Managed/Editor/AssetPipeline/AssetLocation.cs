using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using March.Core.Rendering;
using System.Diagnostics.CodeAnalysis;
using System.Text;

namespace March.Editor.AssetPipeline
{
    public enum AssetCategory
    {
        Unknown,
        ProjectAsset,
        EngineShader,
    }

    public readonly struct AssetLocation
    {
        private const string k_ImporterPathSuffix = ".meta";

        public AssetCategory Category { get; private init; }

        /// <summary>
        /// 这是引擎使用的路径，不是文件系统路径
        /// </summary>
        public string AssetPath { get; private init; }

        public string AssetFullPath { get; private init; }

        /// <summary>
        /// meta 文件的完整路径
        /// </summary>
        public string ImporterFullPath { get; private init; }

        public static AssetLocation FromPath(string path)
        {
            path = path.ValidatePath();

            if (!IsImporterFilePath(path))
            {
                // 需要排除掉 Assets 文件夹本身
                if (path.StartsWith("Assets/", StringComparison.CurrentCultureIgnoreCase))
                {
                    return new()
                    {
                        Category = AssetCategory.ProjectAsset,
                        AssetPath = path,
                        AssetFullPath = AssetLocationUtility.CombinePath(Application.DataPath, path),
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", path + k_ImporterPathSuffix),
                    };
                }

                // 需要排除掉 Engine/Shaders 文件夹本身
                if (path.StartsWith("Engine/Shaders/", StringComparison.CurrentCultureIgnoreCase))
                {
                    return new()
                    {
                        Category = AssetCategory.EngineShader,
                        AssetPath = path,
                        AssetFullPath = AssetLocationUtility.CombinePath(ShaderUtility.EngineShaderPath, path[15..]), // "Engine/Shaders/".Length == 15
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", path + k_ImporterPathSuffix),
                    };
                }
            }

            return new()
            {
                Category = AssetCategory.Unknown,
                AssetPath = path,
                AssetFullPath = path,
                ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", path + k_ImporterPathSuffix),
            };
        }

        public static AssetLocation FromFullPath(string fullPath)
        {
            fullPath = fullPath.ValidatePath();

            if (!IsImporterFilePath(fullPath))
            {
                if (fullPath.StartsWith(Application.DataPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    string relativePath = fullPath[(Application.DataPath.Length + 1)..];

                    return new()
                    {
                        Category = AssetCategory.ProjectAsset,
                        AssetPath = relativePath,
                        AssetFullPath = fullPath,
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", relativePath + k_ImporterPathSuffix),
                    };
                }

                if (fullPath.StartsWith(ShaderUtility.EngineShaderPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    string relativePath = fullPath[(ShaderUtility.EngineShaderPath.Length + 1)..];

                    return new()
                    {
                        Category = AssetCategory.EngineShader,
                        AssetPath = "Engine/Shaders/" + relativePath,
                        AssetFullPath = fullPath,
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", relativePath + k_ImporterPathSuffix),
                    };
                }
            }

            return new()
            {
                Category = AssetCategory.Unknown,
                AssetPath = fullPath,
                AssetFullPath = fullPath,
                ImporterFullPath = fullPath + k_ImporterPathSuffix,
            };
        }

        public static bool IsImporterFilePath(string path) => path.EndsWith(k_ImporterPathSuffix);
    }

    public static class AssetLocationUtility
    {
        private static StringBuilder AppendPath(this StringBuilder builder, StringLike path)
        {
            int initialLength = builder.Length;

            builder.Append(path);
            builder.Replace('\\', '/', initialLength, path.Length);

            while (builder.Length > initialLength && builder[^1] == '/')
            {
                builder.Remove(builder.Length - 1, 1);
            }

            return builder;
        }

        [return: NotNullIfNotNull(nameof(path))]
        public static string? ValidatePath(this string? path)
        {
            if (path != null)
            {
                using var builder = StringBuilderPool.Get();
                builder.Value.AppendPath(path);
                path = builder.Value.ToString();
            }

            return path;
        }

        public static string CombinePath(StringLike path1, StringLike path2)
        {
            using var builder = StringBuilderPool.Get();
            builder.Value.AppendPath(path1).Append('/').AppendPath(path2);
            return builder.Value.ToString();
        }

        public static string CombinePath(StringLike path1, StringLike path2, StringLike path3)
        {
            using var builder = StringBuilderPool.Get();
            builder.Value.AppendPath(path1).Append('/').AppendPath(path2).Append('/').AppendPath(path3);
            return builder.Value.ToString();
        }

        public static string CombinePath(StringLike path1, StringLike path2, StringLike path3, StringLike path4)
        {
            using var builder = StringBuilderPool.Get();
            builder.Value.AppendPath(path1).Append('/').AppendPath(path2).Append('/').AppendPath(path3).Append('/').AppendPath(path4);
            return builder.Value.ToString();
        }

        public static string CombinePath(StringLike path1, StringLike path2, StringLike path3, StringLike path4, StringLike path5)
        {
            using var builder = StringBuilderPool.Get();
            builder.Value.AppendPath(path1).Append('/').AppendPath(path2).Append('/').AppendPath(path3).Append('/').AppendPath(path4).Append('/').AppendPath(path5);
            return builder.Value.ToString();
        }
    }
}
