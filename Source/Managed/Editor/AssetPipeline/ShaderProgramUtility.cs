using March.Core;
using March.Core.Pool;
using March.ShaderLab;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor.AssetPipeline
{
    public static class ShaderProgramUtility
    {
        public static void GetHLSLIncludeFileLocations(string fileFullPath, string source, List<AssetLocation> locations)
        {
            ShaderCompiler.GetHLSLIncludesAndPragmas(source, out List<string> includes, out _);
            GetHLSLIncludeFileLocations(fileFullPath, includes, locations);
        }

        public static void GetHLSLIncludeFileLocations(string fileFullPath, List<string> includePaths, List<AssetLocation> locations)
        {
            using var basePaths = ListPool<string>.Get();
            GetTestBasePaths(fileFullPath, basePaths);

            using var processedPaths = HashSetPool<string>.Get();

            foreach (string incPath in includePaths)
            {
                string path = incPath.ValidatePath();

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

            basePaths.Add(Application.EngineShaderPath);
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

        private sealed class HashEqualityComparer : IEqualityComparer<byte[]>
        {
            public bool Equals(byte[]? x, byte[]? y)
            {
                if (x == null && y == null)
                {
                    return true;
                }

                if (x != null && y != null)
                {
                    return x.SequenceEqual(y);
                }

                return false;
            }

            public int GetHashCode([DisallowNull] byte[] obj)
            {
                var hash = new HashCode();
                hash.AddBytes(obj);
                return hash.ToHashCode();
            }
        }

        private static readonly IEqualityComparer<byte[]> s_HashComparer = new HashEqualityComparer();

        public static IEnumerable<byte[]> GetDistinctProgramHashes(IEnumerable<byte[]> hashes)
        {
            return hashes.Distinct(s_HashComparer);
        }
    }
}
