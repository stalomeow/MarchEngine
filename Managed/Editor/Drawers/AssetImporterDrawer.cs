namespace DX12Demo.Editor.Drawers
{
    public class AssetImporterDrawer : IInspectorDrawerFor<AssetImporter>
    {
        public void Draw(AssetImporter target)
        {
            DrawHeader(target);

            if (EditorGUI.ObjectPropertyFields(target))
            {
                target.SaveAndReimport();
            }
        }

        public static void DrawHeader(AssetImporter target)
        {
            EditorGUI.SeparatorText(target.DisplayName);

            using (new EditorGUI.DisabledScope())
            {
                EditorGUI.LabelField("Path", string.Empty, target.AssetPath);
            }
        }
    }
}
