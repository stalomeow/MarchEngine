using March.Core;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.Serialization;

namespace March.Editor
{
    public abstract class AssetImporter : MarchObject
    {
        /// <summary>
        /// 资产的路径，相对于 <see cref="Application.DataPath"/>
        /// </summary>
        [JsonIgnore]
        public string AssetPath { get; private set; } = string.Empty;

        /// <summary>
        /// 资产的完整路径
        /// </summary>
        [JsonIgnore]
        public string AssetFullPath { get; private set; } = string.Empty;

        /// <summary>
        /// 资产缓存的完整路径
        /// </summary>
        [JsonIgnore]
        internal string AssetCacheFullPath { get; private set; } = string.Empty;

        /// <summary>
        /// <see cref="AssetImporter"/> 文件的完整路径
        /// </summary>
        [JsonIgnore]
        internal string ImporterFullPath { get; private set; } = string.Empty;

        /// <summary>
        /// 资产的 Guid，第一次分配后不会改变
        /// </summary>
        [JsonProperty]
        [HideInInspector]
        public string AssetGuid { get; private set; }

        /// <summary>
        /// 序列化时使用的 <see cref="AssetImporter"/> 版本号
        /// </summary>
        [JsonProperty]
        [HideInInspector]
        private int m_SerializedVersion;

        /// <summary>
        /// 资产最后写入时间，用于判断是否需要重新导入
        /// </summary>
        [JsonProperty]
        [HideInInspector]
        private DateTime? m_AssetLastWriteTimeUtc;

        /// <summary>
        /// 资产的弱引用
        /// </summary>
        [JsonIgnore]
        private WeakReference<MarchObject>? m_AssetWeakRef;

        protected AssetImporter()
        {
            // 生成一个新的 Guid，反序列化时可能会被覆盖
            // https://learn.microsoft.com/en-us/dotnet/api/system.guid.tostring?view=net-8.0
            AssetGuid = System.Guid.NewGuid().ToString("N");

            // 记录版本号，反序列化时可能会被覆盖
            m_SerializedVersion = Version;
        }

        /// <summary>
        /// 导入的资产
        /// </summary>
        [JsonIgnore]
        public MarchObject Asset
        {
            get
            {
                MarchObject asset = GetAssetReference(out bool isEmptyObject);

                if (isEmptyObject)
                {
                    if (NeedReimportAsset())
                    {
                        SaveImporterAndReimportAsset();
                    }
                    else
                    {
                        ImportAsset(asset);
                    }
                }

                return asset;
            }
        }

        internal void Initialize(string assetPath, string assetFullPath, string assetCacheFullPath,
            string importerFullPath, MarchObject? asset = null)
        {
            AssetPath = assetPath;
            AssetFullPath = assetFullPath;
            AssetCacheFullPath = assetCacheFullPath;
            ImporterFullPath = importerFullPath;

            if (asset != null)
            {
                m_AssetWeakRef = new WeakReference<MarchObject>(asset);
                SetAssetMetadata(asset);
            }

            if (NeedReimportAsset())
            {
                SaveImporterAndReimportAsset();
            }
        }

        public virtual bool NeedReimportAsset()
        {
            if (m_SerializedVersion != Version || m_AssetLastWriteTimeUtc != File.GetLastWriteTimeUtc(AssetFullPath))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// 保存 <see cref="AssetImporter"/> 并重新导入资产
        /// </summary>
        [MemberNotNull(nameof(m_AssetLastWriteTimeUtc))]
        public void SaveImporterAndReimportAsset()
        {
            UpdateAndSaveImporter();

            MarchObject asset = GetAssetReference(out _);
            ReimportAsset(asset);

            Debug.LogInfo($"Reimport asset: {AssetPath}");
        }

        [MemberNotNull(nameof(m_AssetLastWriteTimeUtc))]
        protected void UpdateAndSaveImporter()
        {
            m_SerializedVersion = Version;
            RecordAssetLastWriteTime();
            PersistentManager.Save(this, ImporterFullPath);
        }

        [MemberNotNull(nameof(m_AssetLastWriteTimeUtc))]
        private void RecordAssetLastWriteTime()
        {
            m_AssetLastWriteTimeUtc = File.GetLastWriteTimeUtc(AssetFullPath);
        }

        private MarchObject? GetAssetReference()
        {
            if (m_AssetWeakRef != null && m_AssetWeakRef.TryGetTarget(out MarchObject? target))
            {
                return target;
            }

            return null;
        }

        private MarchObject GetAssetReference(out bool isEmptyObject)
        {
            MarchObject? asset = GetAssetReference();

            if (asset == null)
            {
                asset = CreateAsset();
                SetAssetMetadata(asset);
                isEmptyObject = true;

                if (m_AssetWeakRef == null)
                {
                    m_AssetWeakRef = new WeakReference<MarchObject>(asset);
                }
                else
                {
                    m_AssetWeakRef.SetTarget(asset);
                }
            }
            else
            {
                isEmptyObject = false;
            }

            return asset;
        }

        private void SetAssetMetadata(MarchObject asset)
        {
            asset.PersistentGuid = AssetGuid;
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            MarchObject? asset = GetAssetReference();

            if (asset != null)
            {
                SetAssetMetadata(asset);
            }
        }

        /// <summary>
        /// <see cref="AssetImporter"/> 的显示名称，必须返回一个常量
        /// </summary>
        [JsonIgnore]
        public abstract string DisplayName { get; }

        /// <summary>
        /// <see cref="AssetImporter"/> 代码的版本，每次修改代码时都必须增加这个版本号
        /// </summary>
        /// <remarks>子类重写时返回 <c>base.Version + number</c></remarks>
        [JsonIgnore]
        protected virtual int Version => 1;

        /// <summary>
        /// 创建 Asset 实例，只创建实例，不填充数据
        /// </summary>
        /// <returns></returns>
        protected abstract MarchObject CreateAsset();

        /// <summary>
        /// 导入资产，使用之前的缓存
        /// </summary>
        /// <param name="asset">覆写到该实例中</param>
        protected abstract void ImportAsset(MarchObject asset);

        /// <summary>
        /// 重新导入资产，不要使用之前的缓存
        /// </summary>
        /// <param name="asset">覆写到该实例中</param>
        protected abstract void ReimportAsset(MarchObject asset);
    }

    public abstract class DirectAssetImporter : AssetImporter
    {
        protected sealed override void ImportAsset(MarchObject asset)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }

        protected sealed override void ReimportAsset(MarchObject asset)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }

        public void SaveAsset()
        {
            PersistentManager.Save(Asset, AssetFullPath);
            UpdateAndSaveImporter(); // 避免等会又重新导入
        }
    }

    public abstract class ExternalAssetImporter : AssetImporter
    {
        public sealed override bool NeedReimportAsset()
        {
            if (UseCache && !File.Exists(AssetCacheFullPath))
            {
                return true;
            }

            return base.NeedReimportAsset();
        }

        protected sealed override void ImportAsset(MarchObject asset)
        {
            if (UseCache)
            {
                PersistentManager.Overwrite(AssetCacheFullPath, asset);
            }
            else
            {
                PopulateAsset(asset, false);
            }
        }

        protected sealed override void ReimportAsset(MarchObject asset)
        {
            bool willSaveAsset = UseCache;
            PopulateAsset(asset, willSaveAsset);

            if (willSaveAsset)
            {
                PersistentManager.Save(asset, AssetCacheFullPath);
                OnDidSaveAsset(asset);
            }
        }

        /// <summary>
        /// 是否缓存导入结果，必须返回一个常量
        /// </summary>
        [JsonIgnore]
        protected abstract bool UseCache { get; }

        /// <summary>
        /// 填充 Asset 实例
        /// </summary>
        /// <param name="asset">已存在的资产实例</param>
        /// <param name="willSaveToFile">
        /// 是否会将 Asset 保存为文件。如果为 <c>true</c>，则可以在此方法中设置序列化使用的临时数据，
        /// 在 Asset 被保存后会调用 <see cref="OnDidSaveAsset(MarchObject)"/> 清除临时数据
        /// </param>
        /// <returns></returns>
        protected abstract void PopulateAsset(MarchObject asset, bool willSaveToFile);

        /// <summary>
        /// 当 Asset 被保存后调用，可以在此方法中清除序列化使用的临时数据
        /// </summary>
        /// <param name="asset"></param>
        protected virtual void OnDidSaveAsset(MarchObject asset) { }
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public sealed class CustomAssetImporterAttribute(params string[] extensionsIncludingLeadingDot) : Attribute
    {
        /// <summary>
        /// 扩展名，前面有 '.'
        /// </summary>
        public string[] Extensions { get; } = extensionsIncludingLeadingDot;
    }
}
