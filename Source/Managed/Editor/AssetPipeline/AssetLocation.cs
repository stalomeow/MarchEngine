using March.Core;
using March.Core.Rendering;
using System.Diagnostics.CodeAnalysis;

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
                    string fullPath = AssetLocationUtility.CombinePath(Application.DataPath, path);

                    return new()
                    {
                        Category = AssetCategory.ProjectAsset,
                        AssetPath = path,
                        AssetFullPath = fullPath,
                        ImporterFullPath = fullPath + k_ImporterPathSuffix,
                    };
                }

                // 需要排除掉 Engine/Shaders 文件夹本身
                if (path.StartsWith("Engine/Shaders/", StringComparison.CurrentCultureIgnoreCase))
                {
                    return new()
                    {
                        Category = AssetCategory.EngineShader,
                        AssetPath = path,
                        // "Engine/".Length == 7
                        // "Engine/Shaders/".Length == 15
                        AssetFullPath = AssetLocationUtility.CombinePath(Shader.EngineShaderPath, path[15..]),
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "EngineMeta", path[7..] + k_ImporterPathSuffix),
                    };
                }
            }

            return new()
            {
                Category = AssetCategory.Unknown,
                AssetPath = path,
                AssetFullPath = path,
                ImporterFullPath = path + k_ImporterPathSuffix,
            };
        }

        public static AssetLocation FromFullPath(string fullPath)
        {
            fullPath = fullPath.ValidatePath();

            if (!IsImporterFilePath(fullPath))
            {
                if (fullPath.StartsWith(Application.DataPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    return new()
                    {
                        Category = AssetCategory.ProjectAsset,
                        AssetPath = fullPath[(Application.DataPath.Length + 1)..],
                        AssetFullPath = fullPath,
                        ImporterFullPath = fullPath + k_ImporterPathSuffix,
                    };
                }

                if (fullPath.StartsWith(Shader.EngineShaderPath + "/", StringComparison.CurrentCultureIgnoreCase))
                {
                    string relativePath = fullPath[(Shader.EngineShaderPath.Length + 1)..];

                    return new()
                    {
                        Category = AssetCategory.EngineShader,
                        AssetPath = "Engine/Shaders/" + relativePath,
                        AssetFullPath = fullPath,
                        ImporterFullPath = AssetLocationUtility.CombinePath(Application.DataPath, "EngineMeta/Shaders", relativePath + k_ImporterPathSuffix),
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
        [return: NotNullIfNotNull(nameof(path))]
        public static string? ValidatePath(this string? path)
        {
            if (path != null)
            {
                path = path.Replace('\\', '/');
                path = path.TrimEnd('/');
            }

            return path;
        }

        public static string CombinePath(string path1, string path2)
        {
            return Path.Combine(path1, path2).ValidatePath();
        }

        public static string CombinePath(string path1, string path2, string path3)
        {
            return Path.Combine(path1, path2, path3).ValidatePath();
        }

        public static string CombinePath(string path1, string path2, string path3, string path4)
        {
            return Path.Combine(path1, path2, path3, path4).ValidatePath();
        }
    }
}
