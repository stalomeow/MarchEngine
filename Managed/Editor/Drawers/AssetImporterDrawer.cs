using DX12Demo.Core.Serialization;

namespace DX12Demo.Editor.Drawers
{
    public abstract class AssetImporterDrawerFor<T> : InspectorDrawerFor<T> where T : AssetImporter
    {
        private bool m_IsChanged;

        public override void OnCreate()
        {
            base.OnCreate();
            m_IsChanged = false;
        }

        public override void OnDestroy()
        {
            if (m_IsChanged)
            {
                RevertChanges();
                m_IsChanged = false;
            }

            base.OnDestroy();
        }

        public sealed override void Draw()
        {
            EditorGUI.SeparatorText(Target.DisplayName);

            using (new EditorGUI.DisabledScope())
            {
                EditorGUI.LabelField("Path", string.Empty, Target.AssetPath);
            }

            m_IsChanged |= DrawProperties(out bool showApplyRevertButtons);

            if (showApplyRevertButtons)
            {
                EditorGUI.Space();

                float applyButtonWidth = EditorGUI.CalcButtonWidth("Apply");
                float revertButtonWidth = EditorGUI.CalcButtonWidth("Revert");
                float totalWidth = applyButtonWidth + EditorGUI.ItemSpacing.X + revertButtonWidth;
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

            DrawAdditional();
        }

        /// <summary>
        /// 绘制属性面板
        /// </summary>
        /// <param name="showApplyRevertButtons"></param>
        /// <returns>是否有属性发生变化</returns>
        protected abstract bool DrawProperties(out bool showApplyRevertButtons);

        protected virtual void DrawAdditional() { }

        protected abstract void ApplyChanges();

        protected abstract void RevertChanges();
    }

    internal abstract class DirectAssetImporterDrawerFor<T> : AssetImporterDrawerFor<T> where T : DirectAssetImporter
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = EditorGUI.ObjectPropertyFields(Target.Asset, out int propertyCount);
            showApplyRevertButtons = propertyCount > 0;
            return isChanged;
        }

        protected override void ApplyChanges()
        {
            Target.SaveAsset();
        }

        protected override void RevertChanges()
        {
            Target.SaveImporterAndReimportAsset();
        }
    }

    internal sealed class DefaultDirectAssetImporterDrawer : DirectAssetImporterDrawerFor<DirectAssetImporter> { }

    internal abstract class ExternalAssetImporterDrawerFor<T> : AssetImporterDrawerFor<T> where T : ExternalAssetImporter
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = EditorGUI.ObjectPropertyFields(Target, out int propertyCount);
            showApplyRevertButtons = propertyCount > 0;
            return isChanged;
        }

        protected override void ApplyChanges()
        {
            Target.SaveImporterAndReimportAsset();
        }

        protected override void RevertChanges()
        {
            PersistentManager.Overwrite(Target.ImporterFullPath, Target);
        }
    }

    internal sealed class DefaultExternalAssetImporterDrawer : ExternalAssetImporterDrawerFor<ExternalAssetImporter> { }
}
