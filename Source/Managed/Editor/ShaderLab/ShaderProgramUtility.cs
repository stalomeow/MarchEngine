using March.Core.Pool;
using March.Core.Rendering;
using March.Editor.AssetPipeline;
using System.Text.RegularExpressions;

namespace March.Editor.ShaderLab
{
    internal static partial class ShaderProgramUtility
    {
        public static void GetIncludeFiles(string fileFullPath, string source, List<AssetLocation> locations)
        {
            using var basePaths = ListPool<string>.Get();
            GetTestBasePaths(fileFullPath, basePaths);

            using var processedPaths = HashSetPool<string>.Get();

            foreach (Match match in IncludeRegex().Matches(source))
            {
                string path = match.Groups[1].Value.Trim().ValidatePath();

                if (!processedPaths.Value.Add(path))
                {
                    continue;
                }

                string? fullPath = GetFullPath(path, basePaths);

                if (fullPath == null)
                {
                    continue;
                }

                AssetLocation loc = AssetLocation.FromFullPath(fullPath);

                if (loc.Category == AssetCategory.Unknown)
                {
                    continue;
                }

                locations.Add(loc);
            }
        }

        private static void GetTestBasePaths(string fileFullPath, List<string> basePaths)
        {
            string? directory = Path.GetDirectoryName(fileFullPath);

            // 优先检查当前 shader 文件目录
            if (directory != null)
            {
                basePaths.Add(directory);
            }

            basePaths.Add(Shader.EngineShaderPath);
        }

        private static string? GetFullPath(string path, List<string> basePaths)
        {
            foreach (string basePath in basePaths)
            {
                string fullPath = Path.GetFullPath(path, basePath);

                if (File.Exists(fullPath))
                {
                    return fullPath;
                }
            }

            return null;
        }

        [GeneratedRegex("^\\s*#\\s*include\\s*\"(.*)\"", RegexOptions.Multiline | RegexOptions.ECMAScript)]
        private static partial Regex IncludeRegex();
    }
}
