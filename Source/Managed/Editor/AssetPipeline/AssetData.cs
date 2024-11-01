using March.Core;
using March.Core.IconFont;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;

namespace March.Editor.AssetPipeline
{
    [JsonObject(MemberSerialization.OptIn)]
    internal sealed class AssetData
    {
        private const string k_DefaultIcon = FontAwesome6.File;

        // 真正的资产对象 Lazy 加载

        private readonly WeakReference<MarchObject?> m_Asset = new(null, trackResurrection: false);
        private string m_Guid = GuidUtility.GetNew();

        public string Guid
        {
            get => m_Guid;

            internal set
            {
                m_Guid = value;
                MarkAssetPersistent();
            }
        }

        [JsonProperty]
        public string Name { get; set; } = "UnnamedAsset";

        [JsonProperty]
        public string NormalIcon { get; private set; } = k_DefaultIcon;

        [JsonProperty]
        public string ExpandedIcon { get; private set; } = k_DefaultIcon;

        public bool IsAssetCreated => m_Asset.TryGetTarget(out _);

        public bool TryGetAsset([NotNullWhen(true)] out MarchObject? asset)
        {
            return m_Asset.TryGetTarget(out asset);
        }

        public void SetAsset(MarchObject asset)
        {
            MarkAssetPersistent(asset);
            m_Asset.SetTarget(asset);
        }

        public void Reset(MarchObject asset, string? normalIcon, string? expandedIcon)
        {
            SetAsset(asset);
            SetIcons(normalIcon, expandedIcon);
        }

        /// <summary>
        /// 这个方法会尽可能复用原来的资产对象，实现 Hot Reload 效果
        /// </summary>
        /// <param name="assetFactory"></param>
        /// <param name="normalIcon"></param>
        /// <param name="expandedIcon"></param>
        /// <returns></returns>
        public MarchObject Reset(Func<MarchObject> assetFactory, string? normalIcon, string? expandedIcon)
        {
            if (!m_Asset.TryGetTarget(out MarchObject? asset))
            {
                asset = assetFactory();
                SetAsset(asset);
            }

            SetIcons(normalIcon, expandedIcon);
            return asset;
        }

        private void SetIcons(string? normalIcon, string? expandedIcon)
        {
            NormalIcon = normalIcon ?? k_DefaultIcon;
            ExpandedIcon = expandedIcon ?? normalIcon ?? k_DefaultIcon;
        }

        private void MarkAssetPersistent()
        {
            if (m_Asset.TryGetTarget(out MarchObject? asset))
            {
                MarkAssetPersistent(asset);
            }
        }

        private void MarkAssetPersistent(MarchObject asset)
        {
            asset.PersistentGuid = m_Guid;
        }
    }
}
