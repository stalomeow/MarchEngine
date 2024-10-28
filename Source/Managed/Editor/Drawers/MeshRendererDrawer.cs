using March.Core.Rendering;
using Newtonsoft.Json.Serialization;

namespace March.Editor.Drawers
{
    internal class MeshRendererDrawer : IComponentDrawerFor<MeshRenderer>
    {
        public bool Draw(MeshRenderer component, JsonObjectContract contract)
        {
            bool isChanged = false;

            isChanged |= EditorGUI.PropertyField(contract.GetEditorProperty(component, "Mesh"));
            isChanged |= EditorGUI.PropertyField(contract.GetEditorProperty(component, "MeshType"));

            bool areMaterialsChanged = false;

            for (int i = 0; i < component.Mesh.SubMeshCount; i++)
            {
                if (i >= component.Materials.Count)
                {
                    component.Materials.Add(null);
                    areMaterialsChanged = true;
                }

                Material? material = component.Materials[i];

                if (EditorGUI.MarchObjectField($"Material {i}", string.Empty, ref material))
                {
                    component.Materials[i] = material;
                    areMaterialsChanged = true;
                }
            }

            if (areMaterialsChanged)
            {
                isChanged = true;
                component.SyncNativeMaterials();
            }

            return isChanged;
        }
    }
}
