using March.Core.Rendering;
using Newtonsoft.Json.Serialization;

namespace March.Editor.Drawers
{
    internal enum DirectionalLightUnit
    {
        Lux = LightUnit.Lux,
    }

    internal enum PunctualLightUnit
    {
        Lumen = LightUnit.Lumen,
        Candela = LightUnit.Candela,
    }

    internal class LightDrawer : IComponentDrawerFor<Light>
    {
        public bool Draw(Light light, JsonObjectContract contract)
        {
            bool changed = false;

            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "Type"));
            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "UseColorTemperature"));

            if (light.UseColorTemperature)
            {
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ColorTemperature"));
            }
            else
            {
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "Color"));
            }

            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "Intensity"));

            if (light.Type == LightType.Directional)
            {
                DirectionalLightUnit unit = (DirectionalLightUnit)light.Unit;

                if (EditorGUI.EnumField("Unit", "", ref unit))
                {
                    light.Unit = (LightUnit)unit;
                    changed = true;
                }
            }
            else
            {
                PunctualLightUnit unit = (PunctualLightUnit)light.Unit;

                if (EditorGUI.EnumField("Unit", "", ref unit))
                {
                    light.Unit = (LightUnit)unit;
                    changed = true;
                }
            }

            if (light.Type != LightType.Directional)
            {
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "AttenuationRadius"));
            }

            if (light.Type == LightType.Spot)
            {
                float innerConeAngle = light.SpotInnerConeAngle;
                float outerConeAngle = light.SpotOuterConeAngle;

                if (EditorGUI.FloatRangeField("Cone Angle", "", ref innerConeAngle, ref outerConeAngle, minValue: 0.0f, maxValue: 90.0f))
                {
                    light.SpotInnerConeAngle = innerConeAngle;
                    light.SpotOuterConeAngle = outerConeAngle;
                    changed = true;
                }
            }

            return changed;
        }
    }
}
