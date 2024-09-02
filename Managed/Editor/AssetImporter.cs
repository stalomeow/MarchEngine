using DX12Demo.Core;
using DX12Demo.Core.Serialization;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;

namespace DX12Demo.Editor
{
    public abstract class AssetImporter : EngineObject
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
        /// 序列化时使用的 <see cref="AssetImporter"/> 版本号
        /// </summary>
        [JsonProperty]
        [HideInInspector]
        private int? m_SerializedVersion;

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
        private WeakReference<EngineObject>? m_AssetWeakRef;

        protected AssetImporter()
        {
            m_SerializedVersion = Version;
        }

        /// <summary>
        /// 导入的资产
        /// </summary>
        [JsonIgnore]
        public EngineObject Asset
        {
            get
            {
                EngineObject asset = GetAssetReference(out bool isEmptyObject);

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

        internal void Initialize(string assetPath, string assetFullPath, string assetCacheFullPath, string importerFullPath)
        {
            AssetPath = assetPath;
            AssetFullPath = assetFullPath;
            AssetCacheFullPath = assetCacheFullPath;
            ImporterFullPath = importerFullPath;

            if (NeedReimportAsset())
            {
                SaveImporterAndReimportAsset();
            }
        }

        protected virtual bool NeedReimportAsset()
        {
            if (m_SerializedVersion != Version || m_AssetLastWriteTimeUtc != GetAssetLastWriteTimeUtc())
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// 保存 <see cref="AssetImporter"/> 并重新导入资产
        /// </summary>
        [MemberNotNull(nameof(m_SerializedVersion), nameof(m_AssetLastWriteTimeUtc))]
        public void SaveImporterAndReimportAsset()
        {
            m_SerializedVersion = Version;
            m_AssetLastWriteTimeUtc = GetAssetLastWriteTimeUtc();
            PersistentManager.Save(this, ImporterFullPath);

            EngineObject asset = GetAssetReference(out _);
            ReimportAsset(asset);
        }

        private EngineObject GetAssetReference(out bool isEmptyObject)
        {
            if (m_AssetWeakRef != null && m_AssetWeakRef.TryGetTarget(out EngineObject? target))
            {
                isEmptyObject = false;
                return target;
            }

            EngineObject asset = CreateAsset();
            isEmptyObject = true;

            if (m_AssetWeakRef == null)
            {
                m_AssetWeakRef = new WeakReference<EngineObject>(asset);
            }
            else
            {
                m_AssetWeakRef.SetTarget(asset);
            }

            return asset;
        }

        private DateTime GetAssetLastWriteTimeUtc() => File.GetLastWriteTimeUtc(AssetFullPath);

        /// <summary>
        /// <see cref="AssetImporter"/> 的显示名称，必须返回一个常量
        /// </summary>
        [JsonIgnore]
        public abstract string DisplayName { get; }

        /// <summary>
        /// <see cref="AssetImporter"/> 代码的版本，必须返回一个常量，每次修改代码时都必须增加这个版本号
        /// </summary>
        [JsonIgnore]
        protected abstract int Version { get; }

        /// <summary>
        /// 创建 Asset 实例，只创建实例，不填充数据
        /// </summary>
        /// <returns></returns>
        protected abstract EngineObject CreateAsset();

        /// <summary>
        /// 导入资产，使用之前的缓存
        /// </summary>
        /// <param name="asset">覆写到该实例中</param>
        protected abstract void ImportAsset(EngineObject asset);

        /// <summary>
        /// 重新导入资产，不要使用之前的缓存
        /// </summary>
        /// <param name="asset">覆写到该实例中</param>
        protected abstract void ReimportAsset(EngineObject asset);
    }

    public abstract class DirectAssetImporter : AssetImporter
    {
        protected sealed override void ImportAsset(EngineObject asset)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }

        protected sealed override void ReimportAsset(EngineObject asset)
        {
            PersistentManager.Overwrite(AssetFullPath, asset);
        }
    }

    public abstract class ExternalAssetImporter : AssetImporter
    {
        protected sealed override bool NeedReimportAsset()
        {
            if (UseCache && !File.Exists(AssetCacheFullPath))
            {
                return true;
            }

            return base.NeedReimportAsset();
        }

        protected sealed override void ImportAsset(EngineObject asset)
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

        protected sealed override void ReimportAsset(EngineObject asset)
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
        /// 在 Asset 被保存后会调用 <see cref="OnDidSaveAsset(EngineObject)"/> 清除临时数据
        /// </param>
        /// <returns></returns>
        protected abstract void PopulateAsset(EngineObject asset, bool willSaveToFile);

        /// <summary>
        /// 当 Asset 被保存后调用，可以在此方法中清除序列化使用的临时数据
        /// </summary>
        /// <param name="asset"></param>
        protected virtual void OnDidSaveAsset(EngineObject asset) { }
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
