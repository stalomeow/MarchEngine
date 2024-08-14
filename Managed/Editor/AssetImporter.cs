using DX12Demo.Core;
using DX12Demo.Core.Serialization;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;

namespace DX12Demo.Editor
{
    public abstract class AssetImporter : EngineObject, IAssetProvider
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

        /// <summary>
        /// 资产的强引用，只有 <see cref="m_AssetRefCount"/> 大于 0 时不为 <c>null</c>
        /// </summary>
        [JsonIgnore]
        private EngineObject? m_AssetStrongRef;

        /// <summary>
        /// 资产的引用计数
        /// </summary>
        [JsonIgnore]
        private int m_AssetRefCount = 0;

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
                EngineObject? asset = GetAssetReference();

                if (asset == null)
                {
                    if (NeedReimport())
                    {
                        SaveAndReimport(out asset);
                    }
                    else if (UseCache)
                    {
                        asset = PersistentManager.Load(AssetCacheFullPath);
                        SetAssetReference(asset);
                    }
                    else
                    {
                        asset = CreateAsset();
                        SetAssetReference(asset);
                    }
                }

                return asset;
            }
        }

        private bool NeedReimport()
        {
            if (m_SerializedVersion != Version || m_AssetLastWriteTimeUtc != GetAssetLastWriteTimeUtc())
            {
                return true;
            }

            if (UseCache && !File.Exists(AssetCacheFullPath))
            {
                return true;
            }

            return false;
        }

        internal void Initialize(string assetPath, string assetFullPath, string assetCacheFullPath, string importerFullPath)
        {
            AssetPath = assetPath;
            AssetFullPath = assetFullPath;
            AssetCacheFullPath = assetCacheFullPath;
            ImporterFullPath = importerFullPath;

            if (NeedReimport())
            {
                SaveAndReimport();
            }
        }

        /// <summary>
        /// 保存 <see cref="AssetImporter"/> 并重新导入资产
        /// </summary>
        [MemberNotNull(nameof(m_SerializedVersion), nameof(m_AssetLastWriteTimeUtc), nameof(m_AssetWeakRef))]
        public void SaveAndReimport()
        {
            SaveAndReimport(out _);
        }

        [MemberNotNull(nameof(m_SerializedVersion), nameof(m_AssetLastWriteTimeUtc), nameof(m_AssetWeakRef))]
        private void SaveAndReimport(out EngineObject asset)
        {
            m_SerializedVersion = Version;
            m_AssetLastWriteTimeUtc = GetAssetLastWriteTimeUtc();
            PersistentManager.Save(this, ImporterFullPath);

            asset = CreateAsset();
            SetAssetReference(asset);

            if (UseCache)
            {
                PersistentManager.Save(asset, AssetCacheFullPath);
            }
        }

        internal void IncreaseAssetRef()
        {
            ++m_AssetRefCount;

            if (m_AssetRefCount > 0)
            {
                m_AssetStrongRef ??= Asset;
            }
        }

        internal void DecreaseAssetRef()
        {
            --m_AssetRefCount;

            if (m_AssetRefCount <= 0)
            {
                m_AssetStrongRef = null;
            }
        }

        private EngineObject? GetAssetReference()
        {
            if (m_AssetStrongRef != null)
            {
                return m_AssetStrongRef;
            }

            if (m_AssetWeakRef == null)
            {
                return null;
            }

            return m_AssetWeakRef.TryGetTarget(out EngineObject? target) ? target : null;
        }

        [MemberNotNull(nameof(m_AssetWeakRef))]
        private void SetAssetReference(EngineObject asset)
        {
            if (m_AssetWeakRef == null)
            {
                m_AssetWeakRef = new WeakReference<EngineObject>(asset);
            }
            else
            {
                m_AssetWeakRef.SetTarget(asset);
            }

            if (m_AssetRefCount > 0)
            {
                m_AssetStrongRef = asset;
            }
            else
            {
                m_AssetStrongRef = null;
            }
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
        /// 是否缓存 <see cref="CreateAsset"/> 的结果，必须返回一个常量
        /// </summary>
        [JsonIgnore]
        protected abstract bool UseCache { get; }

        /// <summary>
        /// 创建 Asset 实例
        /// </summary>
        /// <returns></returns>
        protected abstract EngineObject CreateAsset();
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
