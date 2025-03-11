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
            bool changed = EditorGUI.PropertyField(contract.GetEditorProperty(light, "Type"));

            if (EditorGUI.Foldout("Shape", ""))
            {
                using (new EditorGUI.IndentedScope())
                {
                    changed |= DrawShapeProperties(light, contract);
                }
            }

            if (EditorGUI.Foldout("Emission", ""))
            {
                using (new EditorGUI.IndentedScope())
                {
                    changed |= DrawEmissionProperties(light, contract);
                }
            }

            if (EditorGUI.Foldout("Shadow", ""))
            {
                using (new EditorGUI.IndentedScope())
                {
                    changed |= DrawShadowProperties(light, contract);
                }
            }

            return changed;
        }

        private static bool DrawShapeProperties(Light light, JsonObjectContract contract)
        {
            bool changed = false;

            if (light.Type == LightType.Directional)
            {
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "AngularDiameter"));
            }
            else
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

        private static bool DrawEmissionProperties(Light light, JsonObjectContract contract)
        {
            bool changed = EditorGUI.PropertyField(contract.GetEditorProperty(light, "UseColorTemperature"));

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

            return changed;
        }

        private static bool DrawShadowProperties(Light light, JsonObjectContract contract)
        {
            bool changed = false;

            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ShadowDepthBias"));
            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ShadowSlopeScaledDepthBias"));
            changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ShadowDepthBiasClamp"));

            return changed;
        }
    }
}
