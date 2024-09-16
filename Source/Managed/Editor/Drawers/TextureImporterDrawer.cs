using March.Core.Rendering;
using March.Editor.Importers;

namespace March.Editor.Drawers
{
    internal class TextureImporterDrawer : ExternalAssetImporterDrawerFor<TextureImporter>
    {
        protected override void DrawAdditional()
        {
            base.DrawAdditional();

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Preview", string.Empty))
            {
                EditorGUI.DrawTexture((Texture)Target.Asset);
            }
        }
    }
}
