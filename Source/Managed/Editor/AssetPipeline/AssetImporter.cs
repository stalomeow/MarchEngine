using March.Core;
using March.Core.Serialization;
using March.Editor.AssetPipeline.Importers;
using Newtonsoft.Json;
using System.Reflection;
using System.Runtime.Serialization;

namespace March.Editor.AssetPipeline
{
    public abstract class AssetImporter : MarchObject
    {
        [JsonProperty, HideInInspector] private int m_SerializedVersion; // 序列化时使用的 AssetImporter 代码版本号
        [JsonProperty, HideInInspector] private DateTime? m_AssetLastWriteTimeUtc; // 资产最后写入时间，用于判断是否需要重新导入
        [JsonProperty, HideInInspector] private string m_MainAssetGuid = string.Empty;
        [JsonProperty, HideInInspector] private Dictionary<string, AssetData> m_GuidToAssetMap = [];

        protected AssetImporter()
        {
            m_SerializedVersion = Version; // 记录版本号，反序列化时可能会被覆盖
        }

        [OnDeserialized]
        private void OnDeserializedCallback(StreamingContext context)
        {
            // AssetData 在序列化时没有保存 Guid，需要重新设置
            foreach (KeyValuePair<string, AssetData> kv in m_GuidToAssetMap)
            {
                kv.Value.Guid = kv.Key;
            }
        }

        public event Action<AssetImporter>? OnWillReimport;
        public event Action<AssetImporter>? OnDidReimport;

        public AssetCategory Category { get; private set; } = AssetCategory.Unknown;

        public string AssetPath { get; private set; } = string.Empty; // 这是引擎使用的路径，不是文件系统路径

        public string AssetFullPath { get; private set; } = string.Empty;

        internal string ImporterFullPath { get; private set; } = string.Empty; // meta 文件的完整路径

        internal void Initialize(AssetCategory category, string assetPath, string assetFullPath, string importerFullPath)
        {
            Category = category;
            AssetPath = assetPath;
            AssetFullPath = assetFullPath;
            ImporterFullPath = importerFullPath;
        }

        internal void Move(AssetCategory category, string assetPath, string assetFullPath, string importerFullPath)
        {
            Initialize(category, assetPath, assetFullPath, importerFullPath);
            SaveAndReimport(force: true);
        }

        protected void SetMainAsset(string name, MarchObject asset, string? normalIcon, string? expandedIcon)
        {
            AssetData data;

            if (string.IsNullOrEmpty(m_MainAssetGuid))
            {
                data = new AssetData();
                m_MainAssetGuid = data.Guid;
                m_GuidToAssetMap.Add(m_MainAssetGuid, data);
            }
            else
            {
                data = m_GuidToAssetMap[m_MainAssetGuid];
            }

            data.Name = name;
            data.Reset(asset, normalIcon, expandedIcon);
        }

        public void SaveAndReimport(bool force)
        {
            if (!force && !NeedReimport)
            {
                return;
            }

            Debug.LogInfo($"Reimport: {AssetPath}");

            SafeInvokeAction(OnWillReimport);
            var context = new AssetImportContext(m_GuidToAssetMap);

            try
            {
                OnImportAssets(ref context);
                context.GetResults(out m_MainAssetGuid, m_GuidToAssetMap);

                foreach (MarchObject asset in context.ImportedAssets)
                {
                    SaveAssetToCache(asset);
                }

                // 保存 Importer，放在最后面的原因：
                // 1. 如果导入失败，不会保存 Importer，下次重新导入时会再次尝试
                // 2. 导入过程中可能写入原始资产文件，这里可以记录到最后一次的写入时间，避免再触发一次导入
                ForceSaveImporter();
            }
            finally
            {
                context.Dispose();
                SafeInvokeAction(OnDidReimport);
            }
        }

        private void SafeInvokeAction(Action<AssetImporter>? action)
        {
            try
            {
                action?.Invoke(this);
            }
            catch (Exception e)
            {
                Debug.LogException(e);
            }
        }

        protected void ForceSaveImporter()
        {
            m_SerializedVersion = Version;
            m_AssetLastWriteTimeUtc = File.GetLastWriteTimeUtc(AssetFullPath);
            PersistentManager.Save(this, ImporterFullPath);
        }

        public MarchObject MainAsset => GetAsset(m_MainAssetGuid)!;

        public string MainAssetNormalIcon => GetAssetNormalIcon(m_MainAssetGuid)!;

        public string MainAssetExpandedIcon => GetAssetExpandedIcon(m_MainAssetGuid)!;

        public MarchObject? GetAsset(string guid)
        {
            bool forceReimport = false;

            for (int retry = 0; retry < 2; retry++)
            {
                SaveAndReimport(forceReimport);

                if (!m_GuidToAssetMap.TryGetValue(guid, out AssetData? data))
                {
                    return null; // 没有这个资产，不用再尝试了
                }

                if (!data.TryGetAsset(out MarchObject? asset))
                {
                    asset = TryLoadAssetFromCache(guid);

                    // 可能是缓存文件被删除了，重新导入再试一次
                    if (asset == null)
                    {
                        forceReimport = true;
                        continue;
                    }

                    data.SetAsset(asset);
                }

                return asset;
            }

            return null;
        }

        public string? GetAssetNormalIcon(string guid)
        {
            return m_GuidToAssetMap.TryGetValue(guid, out AssetData? data) ? data.NormalIcon : null;
        }

        public string? GetAssetExpandedIcon(string guid)
        {
            return m_GuidToAssetMap.TryGetValue(guid, out AssetData? data) ? data.ExpandedIcon : null;
        }

        public string? GetAssetName(string guid)
        {
            return m_GuidToAssetMap.TryGetValue(guid, out AssetData? data) ? data.Name : null;
        }

        public void GetAssetGuids(List<string> guidList)
        {
            foreach (KeyValuePair<string, AssetData> kv in m_GuidToAssetMap)
            {
                guidList.Add(kv.Key);
            }
        }

        public string DisplayName => GetCustomAttribute().DisplayName;

        private int Version => GetCustomAttribute().Version + 4;

        protected virtual bool NeedReimport
        {
            get => m_SerializedVersion != Version || m_AssetLastWriteTimeUtc != File.GetLastWriteTimeUtc(AssetFullPath);
        }

        protected abstract void OnImportAssets(ref AssetImportContext context);

        protected virtual MarchObject? TryLoadAssetFromCache(string guid)
        {
            string fullPath = GetAssetCacheFileFullPath(guid);

            if (!File.Exists(fullPath))
            {
                return null;
            }

            return PersistentManager.Load(fullPath);
        }

        protected virtual void SaveAssetToCache(MarchObject asset)
        {
            string fullPath = GetAssetCacheFileFullPath(asset.PersistentGuid!);
            PersistentManager.Save(asset, fullPath);
        }

        protected static string GetAssetCacheFileFullPath(string guid)
        {
            return Path.Combine(Application.DataPath, "Library/AssetCache", guid).ValidatePath();
        }

        private static readonly Dictionary<string, Type> s_ExtensionToTypeMap = new();
        private static readonly Dictionary<Type, CustomAssetImporterAttribute> s_TypeToAttributeMap = new();
        private static int? s_TypeCacheVersion;

        private static void RebuildTypeCacheIfNeeded()
        {
            if (s_TypeCacheVersion == TypeCache.Version)
            {
                return;
            }

            s_ExtensionToTypeMap.Clear();
            s_TypeToAttributeMap.Clear();

            foreach (Type type in TypeCache.GetTypesDerivedFrom<AssetImporter>())
            {
                var attr = type.GetCustomAttribute<CustomAssetImporterAttribute>();

                if (attr == null)
                {
                    continue;
                }

                foreach (string extension in attr.Extensions)
                {
                    s_ExtensionToTypeMap[extension] = type;
                }

                s_TypeToAttributeMap[type] = attr;
            }

            s_TypeCacheVersion = TypeCache.Version;
        }

        private CustomAssetImporterAttribute GetCustomAttribute()
        {
            RebuildTypeCacheIfNeeded();
            return s_TypeToAttributeMap[GetType()];
        }

        internal static AssetImporter? CreateForPath(string fullPath)
        {
            RebuildTypeCacheIfNeeded();

            // 文件夹需要特殊处理
            if (Directory.Exists(fullPath))
            {
                return new FolderImporter();
            }

            if (s_ExtensionToTypeMap.TryGetValue(Path.GetExtension(fullPath), out Type? type))
            {
                return Activator.CreateInstance(type) as AssetImporter;
            }

            return null;
        }
    }

    public abstract class DirectAssetImporter : AssetImporter
    {
        private const string k_MainAssetName = "MainAsset";

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            MarchObject asset = context.AddAsset(k_MainAssetName, CreateAsset, true, GetNormalIcon(), GetExpandedIcon());
            PersistentManager.Overwrite(AssetFullPath, asset);
        }

        protected override MarchObject? TryLoadAssetFromCache(string guid)
        {
            return PersistentManager.Load(AssetFullPath);
        }

        protected override void SaveAssetToCache(MarchObject asset)
        {
            PersistentManager.Save(asset, AssetFullPath);
        }

        internal void SaveAndReimport(MarchObject asset)
        {
            SetMainAsset(k_MainAssetName, asset, GetNormalIcon(), GetExpandedIcon());
            PersistentManager.Save(asset, AssetFullPath);
            SaveAndReimport(force: true);
        }

        protected abstract MarchObject CreateAsset();

        protected virtual string? GetNormalIcon() => null;

        protected virtual string? GetExpandedIcon() => null;
    }
}
