using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class LightDrawer : IComponentDrawerFor<Light>
    {
        public bool Draw(Component component, JsonObjectContract contract)
        {
            var light = (Light)component;
            bool changed = false;

            changed |= EditorGUI.PropertyField(light, contract.Properties["Type"]);
            changed |= EditorGUI.PropertyField(light, contract.Properties["Color"]);

            if (light.Type != LightType.Directional)
            {
                changed |= EditorGUI.PropertyField("Falloff Range", light, contract.Properties["FalloffRange"]);
            }

            if (light.Type == LightType.Spot)
            {
                changed |= EditorGUI.PropertyField("Spot Power", light, contract.Properties["SpotPower"]);
            }

            return changed;
        }
    }
}
