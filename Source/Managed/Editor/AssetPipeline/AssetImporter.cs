using March.Core;
using March.Core.Diagnostics;
using March.Core.Pool;
using March.Core.Serialization;
using March.Editor.AssetPipeline.Importers;
using Newtonsoft.Json;
using System.Reflection;
using System.Runtime.Serialization;

namespace March.Editor.AssetPipeline
{
    public enum AssetReimportMode
    {
        FastCheck,
        FullCheck,
        Force,
        Dont,
    }

    public abstract class AssetImporter : MarchObject
    {
        [JsonProperty, HideInInspector] private int m_SerializedVersion; // 序列化时使用的 AssetImporter 代码版本号
        [JsonProperty, HideInInspector] private string m_MainAssetGuid = string.Empty;
        [JsonProperty, HideInInspector] private SortedDictionary<string, AssetData> m_GuidToAssetMap = [];

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

        private AssetLocation m_Location;
        private AssetModificationDetector? m_ModificationDetector;
        public event Action<AssetImporter>? OnWillReimport;
        public event Action<AssetImporter>? OnDidReimport;

        public ref readonly AssetLocation Location => ref m_Location;

        internal void InitLocation(in AssetLocation location)
        {
            m_Location = location;
        }

        internal void MoveLocation(in AssetLocation location)
        {
            DeleteImporterFile();
            InitLocation(location);
        }

        internal void DeleteImporterFile()
        {
            if (File.Exists(Location.ImporterFullPath))
            {
                File.Delete(Location.ImporterFullPath);
            }
        }

        internal void DeleteAssetCaches()
        {
            foreach (KeyValuePair<string, AssetData> kv in m_GuidToAssetMap)
            {
                DeleteAssetCache(kv.Key);
            }
        }

        protected void InitMainAsset(string name, MarchObject asset, string? normalIcon, string? expandedIcon)
        {
            if (!string.IsNullOrEmpty(m_MainAssetGuid))
            {
                throw new InvalidOperationException("Main asset is already set");
            }

            var data = new AssetData { Name = name };
            data.Reset(asset, normalIcon, expandedIcon);

            m_MainAssetGuid = data.Guid;
            m_GuidToAssetMap.Add(data.Guid, data);
        }

        public bool ReimportAndSave(AssetReimportMode mode)
        {
            switch (mode)
            {
                case AssetReimportMode.Dont:
                case AssetReimportMode.FastCheck when !CheckNeedReimport(fullCheck: false):
                case AssetReimportMode.FullCheck when !CheckNeedReimport(fullCheck: true):
                    return false;
            }

            Log.Message(LogLevel.Trace, "Reimport asset", $"{Location.AssetPath}");

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

                using var unusedGuids = ListPool<string>.Get();
                context.GetUnusedGuids(unusedGuids);
                foreach (string unusedGuid in unusedGuids.Value)
                {
                    DeleteAssetCache(unusedGuid);
                }

                // 保存 Importer，放在最后面的原因：
                // 1. 如果导入失败，不会保存 Importer，下次重新导入时会再次尝试
                // 2. 导入过程中可能写入原始资产文件，这里可以记录到最后一次的写入时间，避免再触发一次导入
                ForceSaveImporter(context.UserData, context.Dependencies, updateDependencies: true);
                return true;
            }
            finally
            {
                context.Dispose();
                SafeInvokeAction(OnDidReimport);
                LogImportMessages();
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
                Log.Message(LogLevel.Error, "Exception in AssetImporter event callback", $"{e}");
            }
        }

        private void ForceSaveImporter(object? userData, ReadOnlySpan<string> dependencies, bool updateDependencies)
        {
            SaveModificationDetector(userData, dependencies, updateDependencies);

            m_SerializedVersion = Version;
            PersistentManager.Save(this, Location.ImporterFullPath);
        }

        protected void ForceSaveImporter(object? userData)
        {
            ForceSaveImporter(userData, [], updateDependencies: false);
        }

        public string MainAssetGuid => m_MainAssetGuid;

        public MarchObject MainAsset => GetAsset(m_MainAssetGuid)!;

        public string MainAssetNormalIcon => GetAssetNormalIcon(m_MainAssetGuid)!;

        public string MainAssetExpandedIcon => GetAssetExpandedIcon(m_MainAssetGuid)!;

        public MarchObject? GetAsset(string guid)
        {
            AssetReimportMode mode = AssetReimportMode.FastCheck;

            for (int retry = 0; retry < 2; retry++)
            {
                ReimportAndSave(mode);

                if (!m_GuidToAssetMap.TryGetValue(guid, out AssetData? data))
                {
                    return null; // 没有这个资产，不用再尝试了
                }

                if (!data.TryGetAsset(out MarchObject? asset))
                {
                    try
                    {
                        asset = LoadAssetFromCache(guid);
                    }
                    catch
                    {
                        // 递归时，缓存文件被占用
                        // 假设一个资产里有 a 和 b 两个子资产，a 依赖 b
                        // 加载 a 时，需要加载 b，但是**递归**加载 b 时发现 b 的缓存文件没了
                        // 这时要重新导入 a 和 b，但是 a 的缓存文件还在被占用（读），无法写入，所以会抛出异常
                        mode = AssetReimportMode.Force;
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

        public void GetAssets(List<MarchObject> assetList)
        {
            ReimportAndSave(AssetReimportMode.FastCheck);

            // GetAsset 可能会修改 m_GuidToAssetMap，所以先复制一份
            // 因为之前 Reimport 过了，所以 Guid 一定是下面列表里的这些
            using var guids = ListPool<string>.Get();
            GetAssetGuids(guids);

            foreach (string guid in guids.Value)
            {
                assetList.Add(GetAsset(guid)!);
            }
        }

        public void GetDependencies(List<string> dependencies)
        {
            TryLoadModificationDetectorIfNot();
            m_ModificationDetector?.GetDependencies(dependencies);
        }

        public void GetDependencies(List<AssetImporter> dependencies)
        {
            TryLoadModificationDetectorIfNot();
            m_ModificationDetector?.GetDependencies(dependencies);
        }

        /// <summary>
        /// 该对象被保存在 Library 中，而不是 meta 文件里
        /// </summary>
        protected object? UserData
        {
            get
            {
                TryLoadModificationDetectorIfNot();
                return m_ModificationDetector?.UserData;
            }
        }

        public string DisplayName => GetCustomAttribute().DisplayName;

        private int Version => GetCustomAttribute().Version + 11;

        /// <summary>
        /// 
        /// </summary>
        /// <param name="fullCheck">如果为 <see langword="true"/> 则会以性能为代价，进行更全面的检查</param>
        /// <returns></returns>
        protected virtual bool CheckNeedReimport(bool fullCheck)
        {
            if (string.IsNullOrEmpty(m_MainAssetGuid))
            {
                return true;
            }

            if (m_SerializedVersion != Version)
            {
                return true;
            }

            if (fullCheck)
            {
                // 这里涉及大量文件相关的系统调用，速度比较慢，只在 fullCheck 时检查

                TryLoadModificationDetectorIfNot();

                if (m_ModificationDetector == null || m_ModificationDetector.IsModified(in Location))
                {
                    return true;
                }

                foreach (KeyValuePair<string, AssetData> kv in m_GuidToAssetMap)
                {
                    if (!HasValidAssetCache(kv.Key))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        protected abstract void OnImportAssets(ref AssetImportContext context);

        public virtual void LogImportMessages() { }

        protected virtual MarchObject LoadAssetFromCache(string guid)
        {
            string fullPath = GetAssetCacheFileFullPath(guid);
            return PersistentManager.Load(fullPath);
        }

        protected virtual bool HasValidAssetCache(string guid)
        {
            string fullPath = GetAssetCacheFileFullPath(guid);
            return File.Exists(fullPath);
        }

        protected virtual void SaveAssetToCache(MarchObject asset)
        {
            string fullPath = GetAssetCacheFileFullPath(asset.PersistentGuid!);
            PersistentManager.Save(asset, fullPath);
        }

        protected virtual void DeleteAssetCache(string guid)
        {
            string cacheFullPath = GetAssetCacheFileFullPath(guid);

            if (File.Exists(cacheFullPath))
            {
                File.Delete(cacheFullPath);
            }
        }

        private void TryLoadModificationDetectorIfNot()
        {
            if (m_ModificationDetector != null)
            {
                return;
            }

            string path = GetModificationDetectorFileFullPath();

            if (File.Exists(path))
            {
                m_ModificationDetector = PersistentManager.Load<AssetModificationDetector>(path);
            }
        }

        private void SaveModificationDetector(object? userData, ReadOnlySpan<string> dependencies, bool updateDependencies)
        {
            string path = GetModificationDetectorFileFullPath();

            if (!updateDependencies && m_ModificationDetector == null && File.Exists(path))
            {
                // 必须保留旧的依赖关系
                m_ModificationDetector = PersistentManager.Load<AssetModificationDetector>(path);
            }

            m_ModificationDetector ??= new AssetModificationDetector();
            m_ModificationDetector.UserData = userData;
            m_ModificationDetector.SyncAsset(in Location);

            if (updateDependencies)
            {
                m_ModificationDetector.UpdateDependencies(dependencies);
            }

            PersistentManager.Save(m_ModificationDetector, path);
        }

        private string GetModificationDetectorFileFullPath()
        {
            if (string.IsNullOrEmpty(m_MainAssetGuid))
            {
                throw new InvalidOperationException("Main asset is not set");
            }

            string guid = m_MainAssetGuid;
            using var folder = StringBuilderPool.Get();
            folder.Value.Append(guid[0]).Append(guid[1]);
            return AssetLocationUtility.CombinePath(Application.DataPath, "Library", "Modifications", folder, guid);
        }

        protected static string GetAssetCacheFileFullPath(string guid)
        {
            using var folder = StringBuilderPool.Get();
            folder.Value.Append(guid[0]).Append(guid[1]);
            return AssetLocationUtility.CombinePath(Application.DataPath, "Library", "Artifacts", folder, guid);
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
        protected sealed override void OnImportAssets(ref AssetImportContext context)
        {
            MarchObject asset = context.AddMainAsset(CreateAsset, GetNormalIcon(), GetExpandedIcon());
            PersistentManager.Overwrite(Location.AssetFullPath, asset);
        }

        protected sealed override MarchObject LoadAssetFromCache(string guid)
        {
            return PersistentManager.Load(Location.AssetFullPath);
        }

        protected sealed override bool HasValidAssetCache(string guid)
        {
            return true;
        }

        protected sealed override void SaveAssetToCache(MarchObject asset)
        {
            PersistentManager.Save(asset, Location.AssetFullPath);
        }

        protected sealed override void DeleteAssetCache(string guid) { }

        internal void SetAssetAndSave(MarchObject asset)
        {
            InitMainAsset(AssetImportContext.DefaultMainAssetName, asset, GetNormalIcon(), GetExpandedIcon());
            PersistentManager.Save(asset, Location.AssetFullPath);
            ForceSaveImporter(userData: null);
        }

        internal void SaveAsset()
        {
            PersistentManager.Save(MainAsset, Location.AssetFullPath);
            ForceSaveImporter(userData: null); // 记录这次的写入时间，避免等会触发一次重新导入
        }

        protected abstract MarchObject CreateAsset();

        protected virtual string? GetNormalIcon() => null;

        protected virtual string? GetExpandedIcon() => null;
    }

    public abstract class AssetImporterDrawerFor<T> : InspectorDrawerFor<T> where T : AssetImporter
    {
        private bool m_IsChanged;
        private List<MarchObject>? m_EditingAssets; // 保存资产的强引用，避免编辑时被 GC 回收

        /// <summary>
        /// 如果编辑器需要使用实际的资产对象，必须返回 <see langword="true"/>，保证资产不被 GC 回收
        /// </summary>
        protected virtual bool RequireAssets => false;

        public override void OnCreate()
        {
            base.OnCreate();

            m_IsChanged = false;

            if (RequireAssets)
            {
                m_EditingAssets = ListPool<MarchObject>.Shared.Rent();
                Target.GetAssets(m_EditingAssets);
            }
        }

        public override void OnDestroy()
        {
            if (m_IsChanged)
            {
                RevertChanges();
                m_IsChanged = false;
            }

            if (m_EditingAssets != null)
            {
                ListPool<MarchObject>.Shared.Return(m_EditingAssets);
                m_EditingAssets = null;
            }

            base.OnDestroy();
        }

        public sealed override void Draw()
        {
            DrawHeader();

            using (new EditorGUI.DisabledScope(!Target.Location.IsEditable))
            {
                EditorGUI.Space();
                m_IsChanged |= DrawProperties(out bool showApplyRevertButtons);

                if (showApplyRevertButtons)
                {
                    EditorGUI.Space();
                    DrawApplyRevertButtons();
                }
            }

            EditorGUI.Space();
            DrawAdditional();
        }

        private void DrawHeader()
        {
            using var label = EditorGUIUtility.BuildAssetPath(Target, useCompleteAssetPath: false);
            label.Value.Append(' ').Append('(').Append(' ').Append(Target.DisplayName).Append(' ').Append(')');
            EditorGUI.SeparatorText(label);
        }

        private void DrawApplyRevertButtons()
        {
            ReadOnlySpan<float> widths = [
                EditorGUI.CalcButtonWidth("Apply"),
                EditorGUI.CalcButtonWidth("Revert"),
            ];

            float totalWidth = widths.Sum() + (widths.Length - 1) * EditorGUI.ItemSpacing.X;
            EditorGUI.CursorPosX += EditorGUI.ContentRegionAvailable.X - totalWidth;

            using (new EditorGUI.DisabledScope(!m_IsChanged))
            {
                if (EditorGUI.Button("Apply"))
                {
                    ApplyChanges();
                    m_IsChanged = false;
                }

                EditorGUI.SameLine();

                if (EditorGUI.Button("Revert"))
                {
                    RevertChanges();
                    m_IsChanged = false;
                }
            }
        }

        /// <summary>
        /// 绘制属性面板
        /// </summary>
        /// <param name="showApplyRevertButtons"></param>
        /// <returns>是否有属性发生变化</returns>
        protected virtual bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = EditorGUI.ObjectPropertyFields(Target, out int propertyCount);
            showApplyRevertButtons = propertyCount > 0;
            return isChanged;
        }

        protected virtual void DrawAdditional()
        {
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Artifacts", ""))
            {
                using var guids = ListPool<string>.Get();
                Target.GetAssetGuids(guids);

                guids.Value.Sort((x, y) =>
                {
                    string name1 = Target.GetAssetName(x)!;
                    string name2 = Target.GetAssetName(y)!;
                    return StringComparer.InvariantCulture.Compare(name1, name2);
                });

                using (new EditorGUI.IndentedScope())
                {
                    foreach (string guid in guids.Value)
                    {
                        using var label = StringBuilderPool.Get();
                        label.Value.Append(Target.GetAssetNormalIcon(guid)!);
                        label.Value.Append(' ');
                        label.Value.Append(Target.GetAssetName(guid)!);
                        label.Value.Append(' ').Append('<').Append(guid).Append('>');
                        EditorGUI.BulletLabel(label, "");
                    }
                }
            }

            if (EditorGUI.Foldout("Dependencies", ""))
            {
                using var dependencies = ListPool<AssetImporter>.Get();
                Target.GetDependencies(dependencies);

                using (new EditorGUI.IndentedScope())
                {
                    foreach (AssetImporter dep in dependencies.Value)
                    {
                        using var depLabel = EditorGUIUtility.BuildAssetPath(dep);
                        EditorGUI.BulletLabel(depLabel, "");
                    }
                }
            }
        }

        protected virtual void ApplyChanges()
        {
            Target.ReimportAndSave(AssetReimportMode.Force);
        }

        protected virtual void RevertChanges()
        {
            PersistentManager.Overwrite(Target.Location.ImporterFullPath, Target);
        }
    }

    file sealed class DefaultAssetImporterDrawer : AssetImporterDrawerFor<AssetImporter> { }

    public abstract class DirectAssetImporterDrawerFor<T> : AssetImporterDrawerFor<T> where T : DirectAssetImporter
    {
        protected override bool RequireAssets => true;

        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = EditorGUI.ObjectPropertyFields(Target.MainAsset, out int propertyCount);
            showApplyRevertButtons = propertyCount > 0;
            return isChanged;
        }

        protected override void ApplyChanges()
        {
            Target.SaveAsset();
        }

        protected override void RevertChanges()
        {
            Target.ReimportAndSave(AssetReimportMode.Force);
        }
    }

    file sealed class DefaultDirectAssetImporterDrawer : DirectAssetImporterDrawerFor<DirectAssetImporter> { }
}
