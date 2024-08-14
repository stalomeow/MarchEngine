using DX12Demo.Core.Serialization;

namespace DX12Demo.Editor.Drawers
{
    public class AssetImporterDrawer<T> : InspectorDrawerFor<T> where T : AssetImporter
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

            m_IsChanged |= DrawProperties(out bool hasProperty);

            if (hasProperty)
            {
                EditorGUI.Space();

                using (new EditorGUI.DisabledScope(!m_IsChanged))
                {
                    if (EditorGUI.Button("Apply"))
                    {
                        ApplyChanges();
                    }

                    EditorGUI.SameLine();

                    if (EditorGUI.Button("Revert"))
                    {
                        RevertChanges();
                    }
                }
            }

            DrawAdditional();
        }

        private void ApplyChanges()
        {
            Target.SaveAndReimport();
            m_IsChanged = false;
        }

        private void RevertChanges()
        {
            PersistentManager.Overwrite(Target.ImporterFullPath, Target);
            m_IsChanged = false;
        }

        protected virtual bool DrawProperties(out bool hasProperty)
        {
            bool isChanged = EditorGUI.ObjectPropertyFields(Target, out int propertyCount);
            hasProperty = propertyCount > 0;
            return isChanged;
        }

        protected virtual void DrawAdditional() { }
    }

    internal sealed class DefaultAssetImporterDrawer : AssetImporterDrawer<AssetImporter> { }
}
