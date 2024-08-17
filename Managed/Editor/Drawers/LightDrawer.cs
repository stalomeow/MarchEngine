using DX12Demo.Core.Rendering;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class LightDrawer : IComponentDrawerFor<Light>
    {
        public bool Draw(Light light, JsonObjectContract contract)
        {
            bool changed = false;

            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "Type"));
            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "Color"));

            if (light.Type != LightType.Directional)
            {
                changed |= EditorGUI.PropertyField("Falloff Range", contract.GetEditorProperty(light, "FalloffRange"));
            }

            if (light.Type == LightType.Spot)
            {
                changed |= EditorGUI.PropertyField("Spot Power", contract.GetEditorProperty(light, "SpotPower"));
            }

            return changed;
        }
    }
}
