using March.Core;
using March.Core.Diagnostics;
using Newtonsoft.Json;
using System.Runtime.CompilerServices;

namespace March.Editor.AssetPipeline
{
    internal sealed class AssetModificationDetector : MarchObject
    {
        [method: JsonConstructor]
        private record struct AssetDependency(string Path, DateTime ImporterLastWriteTimeUtc);

        [JsonProperty] private DateTime m_AssetLastWriteTimeUtc;
        [JsonProperty] private List<AssetDependency> m_Dependencies = []; // 记录依赖的 Importer 的修改时间，不是 Asset 的修改时间

        public bool IsModified(in AssetLocation location)
        {
            if (m_AssetLastWriteTimeUtc != GetLastWriteTimeUtc(location.AssetFullPath))
            {
                return true;
            }

            foreach (AssetDependency dep in m_Dependencies)
            {
                AssetImporter? importer = AssetDatabase.GetAssetImporter(dep.Path, AssetReimportMode.Dont);

                if (importer != null && dep.ImporterLastWriteTimeUtc != GetLastWriteTimeUtc(importer.Location.ImporterFullPath))
                {
                    return true;
                }
            }

            return false;
        }

        public void SyncAsset(in AssetLocation location)
        {
            m_AssetLastWriteTimeUtc = GetLastWriteTimeUtc(location.AssetFullPath);
        }

        public void UpdateDependencies(ReadOnlySpan<string> dependencyPaths)
        {
            m_Dependencies.Clear();

            foreach (string path in dependencyPaths)
            {
                AssetImporter? importer = AssetDatabase.GetAssetImporter(path, AssetReimportMode.Dont);

                if (importer == null)
                {
                    Log.Message(LogLevel.Warning, "Asset dependency is not found", $"{path}");
                    continue;
                }

                m_Dependencies.Add(new AssetDependency(path, GetLastWriteTimeUtc(importer.Location.ImporterFullPath)));
            }
        }

        public void GetDependencies(List<string> dependencies)
        {
            foreach (AssetDependency dep in m_Dependencies)
            {
                dependencies.Add(dep.Path);
            }
        }

        public void GetDependencies(List<AssetImporter> dependencies)
        {
            foreach (AssetDependency dep in m_Dependencies)
            {
                AssetImporter? importer = AssetDatabase.GetAssetImporter(dep.Path, AssetReimportMode.Dont);

                if (importer != null)
                {
                    dependencies.Add(importer);
                }
            }
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static DateTime GetLastWriteTimeUtc(string path) => File.GetLastWriteTimeUtc(path);
    }
}
