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

            EditorGUI.LabelField("Warnings", string.Empty, shader.GetWarnings().Count.ToString());
            EditorGUI.LabelField("Errors", string.Empty, shader.GetErrors().Count.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                foreach (string warning in shader.GetWarnings())
                {
                    Debug.LogWarning(warning);
                }

                foreach (string error in shader.GetErrors())
                {
                    Debug.LogError(error);
                }
            }

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Properties", string.Empty))
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
