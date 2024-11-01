using March.Core;
using March.Core.Pool;
using System.Runtime.InteropServices;

namespace March.Editor.AssetPipeline
{
    public ref struct AssetImportContext
    {
        private string? m_MainAssetName;
        private readonly PooledObject<List<MarchObject>> m_ImportedAssets; // 保存强引用，避免 Import 时被 GC 回收
        private readonly PooledObject<Dictionary<string, AssetData>> m_ImportedData; // 使用 name 作为 key
        private readonly PooledObject<Dictionary<string, AssetData>> m_DataPool; // 使用 name 作为 key

        internal AssetImportContext(Dictionary<string, AssetData> oldGuidToAssetMap)
        {
            m_MainAssetName = null;
            m_ImportedAssets = ListPool<MarchObject>.Get();
            m_ImportedData = DictionaryPool<string, AssetData>.Get();
            m_DataPool = DictionaryPool<string, AssetData>.Get();

            foreach (KeyValuePair<string, AssetData> kv in oldGuidToAssetMap)
            {
                m_DataPool.Value.Add(kv.Value.Name, kv.Value);
            }
        }

        public readonly ReadOnlySpan<MarchObject> ImportedAssets => CollectionsMarshal.AsSpan(m_ImportedAssets.Value);

        internal readonly void GetResults(out string mainAssetGuid, Dictionary<string, AssetData> guidToAssetMap)
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
        }

        public T AddAsset<T>(string name, bool isMainAsset, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject, new()
        {
            return AddAsset(name, () => new T(), isMainAsset, normalIcon, expandedIcon);
        }

        public T AddAsset<T>(string name, Func<T> factory, bool isMainAsset, string? normalIcon = null, string? expandedIcon = null) where T : MarchObject
        {
            if (m_ImportedData.Value.TryGetValue(name, out AssetData? data))
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

            if (!m_DataPool.Value.Remove(name, out data))
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
    }
}
