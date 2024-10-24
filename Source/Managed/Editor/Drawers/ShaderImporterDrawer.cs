using March.Core;
using March.Core.Rendering;
using March.Editor.Importers;

namespace March.Editor.Drawers
{
    internal class ShaderImporterDrawer : ExternalAssetImporterDrawerFor<ShaderImporter>
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = base.DrawProperties(out showApplyRevertButtons);

            Shader shader = (Shader)Target.Asset;

            EditorGUI.LabelField("Warnings", string.Empty, shader.Warnings.Length.ToString());
            EditorGUI.LabelField("Errors", string.Empty, shader.Errors.Length.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                foreach (string warning in shader.Warnings)
                {
                    Debug.LogWarning(warning);
                }

                foreach (string error in shader.Errors)
                {
                    Debug.LogError(error);
                }
            }

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Properties", string.Empty, defaultOpen: true))
            {
                using (new EditorGUI.IndentedScope())
                {
                    foreach (ShaderProperty prop in shader.Properties)
                    {
                        EditorGUI.LabelField(prop.Name, string.Empty, prop.Type.ToString());
                    }
                }
            }

            return isChanged;
        }
    }
}
