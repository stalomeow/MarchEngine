using March.Core.Rendering;
using March.Editor.IconFont;
using Newtonsoft.Json.Serialization;
using System.Numerics;

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

    internal class LightDrawer : IComponentDrawerFor<Light>, IGizmoDrawerFor<Light>
    {
        bool IComponentDrawerFor<Light>.Draw(Light light, JsonObjectContract contract)
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

            // 目前只有 Directional Light 支持阴影
            if (light.Type == LightType.Directional && EditorGUI.Foldout("Shadow", ""))
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
            bool changed = EditorGUI.PropertyField(contract.GetEditorProperty(light, "IsCastingShadow"));

            using (new EditorGUI.DisabledScope(!light.IsCastingShadow))
            {
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ShadowDepthBias"));
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ShadowSlopeScaledDepthBias"));
                changed |= EditorGUI.PropertyField(contract.GetEditorProperty(light, "ShadowDepthBiasClamp"));
            }

            return changed;
        }

        void IGizmoDrawerFor<Light>.Draw(Light light, bool isSelected)
        {
            if (!isSelected)
            {
                return;
            }

            switch (light.Type)
            {
                case LightType.Point:
                    DrawPointLightGizmos(light);
                    break;

                case LightType.Spot:
                    DrawSpotLightGizmos(light);
                    break;
            }
        }

        void IGizmoDrawerFor<Light>.DrawGUI(Light light, bool isSelected)
        {
            // Icon
            using (new Gizmos.ColorScope(light.Color))
            {
                Gizmos.DrawText(light.transform.Position, light.Type switch
                {
                    LightType.Directional => FontAwesome6.Sun,
                    LightType.Point => FontAwesome6.Lightbulb,
                    LightType.Spot => FontAwesome6.LandMineOn,
                    _ => throw new NotSupportedException()
                });
            }

            if (isSelected && light.Type == LightType.Directional)
            {
                DrawDirectionLightGizmos(light);
            }
        }

        private static void DrawDirectionLightGizmos(Light light)
        {
            // Directional Light 使用 GUI 模式绘制，大小不受 Camera 到它距离的影响
            const int rayCount = 8;

            using (new Gizmos.ColorScope(light.Color))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(light.transform)))
            {
                float guiScale = Gizmos.GUIScaleAtOrigin;
                float radius = guiScale * 0.15f;
                float rayLength = guiScale * 0.6f;

                Gizmos.DrawWireDisc(Vector3.Zero, Vector3.UnitZ, radius);

                for (int i = 0; i < rayCount; i++)
                {
                    float radians = MathF.PI * 2 / rayCount * i;
                    Vector3 pos = new(MathF.Sin(radians) * radius, MathF.Cos(radians) * radius, 0);
                    Gizmos.DrawLine(pos, pos + Vector3.UnitZ * rayLength);
                }
            }
        }

        private static void DrawPointLightGizmos(Light light)
        {
            using (new Gizmos.ColorScope(light.Color))
            {
                Gizmos.DrawWireSphere(light.transform.Position, light.AttenuationRadius);
            }
        }

        private static void DrawSpotLightGizmos(Light light)
        {
            static void drawCone(float attenuationRadius, float coneAngleInRadians, bool drawArcs)
            {
                float radius = MathF.Sin(coneAngleInRadians) * attenuationRadius;

                if (radius <= 0.001f)
                {
                    return;
                }

                Vector3 center = Vector3.UnitZ * (MathF.Cos(coneAngleInRadians) * attenuationRadius);
                Gizmos.DrawWireDisc(center, Vector3.UnitZ, radius);

                // 类似 UE 的一堆射线
                const int numRays = 16;
                for (int i = 0; i < numRays; i++)
                {
                    float theta = MathF.PI * 2 / numRays * i;
                    float x = MathF.Cos(theta) * radius;
                    float y = MathF.Sin(theta) * radius;

                    Vector3 pos = center + new Vector3(x, y, 0);
                    Gizmos.DrawLine(Vector3.Zero, pos);
                }

                if (drawArcs)
                {
                    Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitX, Vector3.UnitZ, coneAngleInRadians, attenuationRadius);
                    Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitX, Vector3.UnitZ, -coneAngleInRadians, attenuationRadius);
                    Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitY, Vector3.UnitZ, coneAngleInRadians, attenuationRadius);
                    Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitY, Vector3.UnitZ, -coneAngleInRadians, attenuationRadius);
                }
            }

            using (new Gizmos.ColorScope(light.Color))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(light.transform)))
            {
                drawCone(light.AttenuationRadius, float.DegreesToRadians(light.SpotInnerConeAngle), false);
                drawCone(light.AttenuationRadius, float.DegreesToRadians(light.SpotOuterConeAngle), true);
            }
        }
    }
}
