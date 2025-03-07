using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using System.Diagnostics.CodeAnalysis;
using System.Text;

namespace March.Editor.AssetPipeline
{
    public enum AssetCategory
    {
        Unknown,
        ProjectAsset,
        EngineShader,
        EngineResource,
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

        public bool IsEngineBuiltIn => Category is (AssetCategory.EngineShader or AssetCategory.EngineResource);

        public bool IsEditable => Category switch
        {
            AssetCategory.EngineShader => Application.IsEngineShaderEditable,
            AssetCategory.EngineResource => Application.IsEngineResourceEditable,
            _ => true,
        };

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
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", path[7..] + k_ImporterPathSuffix), // "Assets/".Length == 7
                    };
                }

                // 需要排除掉 Engine/Shaders 文件夹本身
                if (path.StartsWith("Engine/Shaders/", StringComparison.CurrentCultureIgnoreCase))
                {
                    return new()
                    {
                        Category = AssetCategory.EngineShader,
                        AssetPath = path,
                        AssetFullPath = AssetLocationUtility.CombinePath(Application.EngineShaderPath, path[15..]), // "Engine/Shaders/".Length == 15
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.EngineResourcePath, "Meta", path[7..] + k_ImporterPathSuffix), // "Engine/".Length == 7
                    };
                }

                // 需要排除掉 Engine/Resources 文件夹本身
                if (path.StartsWith("Engine/Resources/", StringComparison.CurrentCultureIgnoreCase))
                {
                    return new()
                    {
                        Category = AssetCategory.EngineResource,
                        AssetPath = path,
                        AssetFullPath = AssetLocationUtility.CombinePath(Application.EngineResourcePath, "Assets", path[17..]), // "Engine/Resources/".Length == 17
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.EngineResourcePath, "Meta", path[7..] + k_ImporterPathSuffix), // "Engine/".Length == 7
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
                if (TryGetRelativePath(AssetCategory.ProjectAsset, fullPath, out string? relativePath1))
                {
                    return new()
                    {
                        Category = AssetCategory.ProjectAsset,
                        AssetPath = "Assets/" + relativePath1,
                        AssetFullPath = fullPath,
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "Meta", relativePath1 + k_ImporterPathSuffix),
                    };
                }

                if (TryGetRelativePath(AssetCategory.EngineShader, fullPath, out string? relativePath2))
                {
                    return new()
                    {
                        Category = AssetCategory.EngineShader,
                        AssetPath = "Engine/Shaders/" + relativePath2,
                        AssetFullPath = fullPath,
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.EngineResourcePath, "Meta", "Shaders", relativePath2 + k_ImporterPathSuffix),
                    };
                }

                if (TryGetRelativePath(AssetCategory.EngineResource, fullPath, out string? relativePath3))
                {
                    return new()
                    {
                        Category = AssetCategory.EngineResource,
                        AssetPath = "Engine/Resources/" + relativePath3,
                        AssetFullPath = fullPath,
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.EngineResourcePath, "Meta", "Resources", relativePath3 + k_ImporterPathSuffix),
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

        private static bool TryGetRelativePath(AssetCategory category, string fullPath, [NotNullWhen(true)] out string? relativePath)
        {
            string? basePath = GetBaseFullPath(category);

            if (basePath != null && fullPath.StartsWith(basePath + "/", StringComparison.CurrentCultureIgnoreCase))
            {
                relativePath = fullPath[(basePath.Length + 1)..];
                return true;
            }

            relativePath = null;
            return false;
        }

        public static string? GetBaseFullPath(AssetCategory category) => category switch
        {
            AssetCategory.ProjectAsset => AssetLocationUtility.CombinePath(Application.DataPath, "Assets"),
            AssetCategory.EngineShader => Application.EngineShaderPath,
            AssetCategory.EngineResource => AssetLocationUtility.CombinePath(Application.EngineResourcePath, "Assets"),
            _ => null,
        };
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
