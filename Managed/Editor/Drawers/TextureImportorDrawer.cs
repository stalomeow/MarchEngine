using DX12Demo.Core.Rendering;
using DX12Demo.Editor.Importers;

namespace DX12Demo.Editor.Drawers
{
    internal class TextureImportorDrawer : AssetImporterDrawer<TextureImporter>
    {
        protected override void DrawAdditional()
        {
            base.DrawAdditional();

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Preview"))
            {
                EditorGUI.DrawTexture((Texture)Target.Asset);
            }
        }
    }
}
