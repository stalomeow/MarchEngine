using March.Core;
using March.Core.Pool;
using System.Runtime.InteropServices;

namespace March.Editor.AssetPipeline
{
    public ref struct AssetImportContext
    {
        private object? m_UserData;
        private string? m_MainAssetName;
        private readonly PooledObject<List<MarchObject>> m_ImportedAssets; // 保存强引用，避免 Import 时被 GC 回收
        private readonly PooledObject<Dictionary<string, AssetData>> m_ImportedData; // 使用 name 作为 key
        private readonly PooledObject<Dictionary<string, AssetData>> m_DataPool; // 使用 name 作为 key
        private readonly PooledObject<List<string>> m_Dependencies;

        internal AssetImportContext(IReadOnlyDictionary<string, AssetData> oldGuidToAssetMap)
        {
            m_UserData = null;
            m_MainAssetName = null;
            m_ImportedAssets = ListPool<MarchObject>.Get();
            m_ImportedData = DictionaryPool<string, AssetData>.Get();
            m_DataPool = DictionaryPool<string, AssetData>.Get();
            m_Dependencies = ListPool<string>.Get();

            foreach (KeyValuePair<string, AssetData> kv in oldGuidToAssetMap)
            {
                m_DataPool.Value.Add(kv.Value.Name, kv.Value);
            }
        }

        public readonly object? UserData => m_UserData;

        public readonly ReadOnlySpan<MarchObject> ImportedAssets => CollectionsMarshal.AsSpan(m_ImportedAssets.Value);

        public readonly ReadOnlySpan<string> Dependencies => CollectionsMarshal.AsSpan(m_Dependencies.Value);

        internal readonly void GetUnusedGuids(List<string> guids)
        {
            foreach (KeyValuePair<string, AssetData> kv in m_DataPool.Value)
            {
                guids.Add(kv.Value.Guid);
            }
        }

        internal readonly void GetResults(out string mainAssetGuid, IDictionary<string, AssetData> guidToAssetMap)
        {
            if (m_MainAssetName == null)
            {
                throw new InvalidOperationException("Main asset is not set");
            }

            mainAssetGuid = m_ImportedData.Value[m_MainAssetName].Guid;
            guidToAssetMap.Clear();

            foreach (KeyValuePair<string, AssetData> kv in m_ImportedData.Value)
            {
                guidToAssetMap.Add(kv.Value.Guid, kv.Value);
            }
        }

        internal readonly void Dispose()
        {
            m_ImportedAssets.Dispose();
            m_ImportedData.Dispose();
            m_DataPool.Dispose();
            m_Dependencies.Dispose();
        }

        public T AddAsset<T>(string name, bool isMainAsset, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject, new()
        {
            return AddAsset(name, () => new T(), isMainAsset, normalIcon, expandedIcon);
        }

        public T AddAsset<T>(string name, Func<T> factory, bool isMainAsset, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject
        {
            if (m_ImportedData.Value.ContainsKey(name))
            {
                throw new InvalidOperationException("Asset with the same name already exists");
            }

            if (isMainAsset)
            {
                if (m_MainAssetName != null)
                {
                    throw new InvalidOperationException("Main asset is already set");
                }

                m_MainAssetName = name;
            }

            if (!m_DataPool.Value.Remove(name, out AssetData? data))
            {
                data = new AssetData { Name = name };
            }

            var result = (T)data.Reset(factory, normalIcon, expandedIcon);

            m_ImportedAssets.Value.Add(result);
            m_ImportedData.Value.Add(name, data);
            return result;
        }

        public const string DefaultMainAssetName = "Main";

        public T AddMainAsset<T>(string name = DefaultMainAssetName, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject, new()
        {
            return AddAsset<T>(name, true, normalIcon, expandedIcon);
        }

        public T AddMainAsset<T>(Func<T> factory, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject
        {
            return AddMainAsset(DefaultMainAssetName, factory, normalIcon, expandedIcon);
        }

        public T AddMainAsset<T>(string name, Func<T> factory, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject
        {
            return AddAsset(name, factory, true, normalIcon, expandedIcon);
        }

        public T AddSubAsset<T>(string name, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject, new()
        {
            return AddAsset<T>(name, false, normalIcon, expandedIcon);
        }

        public T AddSubAsset<T>(string name, Func<T> factory, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject
        {
            return AddAsset(name, factory, false, normalIcon, expandedIcon);
        }

        public readonly T RequireOtherAsset<T>(string path, bool dependsOn) where T : MarchObject
        {
            return RequireOtherAssetImpl(path, dependsOn, AssetDatabase.Load<T>(path));
        }

        public readonly T RequireOtherAsset<T>(string path, bool dependsOn, Action<AssetImporter> settings) where T : MarchObject
        {
            return RequireOtherAssetImpl(path, dependsOn, AssetDatabase.Reload<T>(path, settings));
        }

        private readonly T RequireOtherAssetImpl<T>(string path, bool dependsOn, T? asset) where T : MarchObject
        {
            if (asset == null)
            {
                throw new FileNotFoundException($"Asset not found at path: {path}");
            }

            if (dependsOn && !m_Dependencies.Value.Contains(path))
            {
                m_Dependencies.Value.Add(path);
            }

            return asset;
        }

        public void SetUserData(object? userData)
        {
            m_UserData = userData;
        }
    }
}
