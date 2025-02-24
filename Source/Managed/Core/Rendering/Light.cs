using March.Core.IconFont;
using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Rendering
{
    public enum LightType
    {
        Directional,
        Point,
        Spot,
    };

    public enum LightUnit
    {
        Lux,
        Lumen,
        Candela,
    }

    public partial class Light : Component
    {
        public Light() : base(New()) { }

        // TODO 检查属性反序列化的顺序。属性在赋值时可能影响其他属性的值，所以要检查赋值的顺序

        [JsonProperty]
        [NativeProperty]
        public partial LightType Type { get; set; }

        [JsonProperty]
        [NativeProperty]
        [ColorDrawer(Alpha = false, HDR = true)]
        public partial Color Color { get; set; }

        [JsonProperty]
        [InspectorName("Use Temperature")]
        [NativeProperty]
        public partial bool UseColorTemperature { get; set; }

        [JsonProperty]
        [InspectorName("Temperature")]
        [FloatDrawer(Min = 1000.0f, Max = 40000.0f, Slider = true)]
        [NativeProperty]
        public partial float ColorTemperature { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial LightUnit Unit { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial float Intensity { get; set; }

        [JsonProperty]
        [InspectorName("Attenuation Radius")]
        [FloatDrawer(Min = 0.0f)]
        [NativeProperty]
        public partial float AttenuationRadius { get; set; }

        // Note: 代码中的 Cone Angle 是圆锥顶角的一半，使用角度制

        [JsonProperty]
        [InspectorName("Inner Cone Angle")]
        [FloatDrawer(Min = 0.0f, Max = 90.0f, Slider = true)]
        [NativeProperty]
        public partial float SpotInnerConeAngle { get; set; } // in degrees

        [JsonProperty]
        [InspectorName("Outer Cone Angle")]
        [FloatDrawer(Min = 0.0f, Max = 90.0f, Slider = true)]
        [NativeProperty]
        public partial float SpotOuterConeAngle { get; set; } // in degrees

        protected override void OnEnable()
        {
            base.OnEnable();
            RenderPipeline.Add(this);
        }

        protected override void OnDisable()
        {
            RenderPipeline.Remove(this);
            base.OnDisable();
        }

        protected override void OnDrawGizmos(bool isSelected)
        {
            base.OnDrawGizmos(isSelected);

            if (isSelected)
            {
                switch (Type)
                {
                    case LightType.Point:
                        DrawPointLightGizmos();
                        break;

                    case LightType.Spot:
                        DrawSpotLightGizmos();
                        break;
                }
            }
        }

        protected override void OnDrawGizmosGUI(bool isSelected)
        {
            base.OnDrawGizmosGUI(isSelected);

            // Icon
            using (new Gizmos.ColorScope(Color))
            {
                Gizmos.DrawText(transform.Position, Type switch
                {
                    LightType.Directional => FontAwesome6.Sun,
                    LightType.Point => FontAwesome6.Lightbulb,
                    LightType.Spot => FontAwesome6.LandMineOn,
                    _ => throw new NotSupportedException()
                });
            }

            if (isSelected && Type == LightType.Directional)
            {
                DrawDirectionLightGizmos();
            }
        }

        private void DrawDirectionLightGizmos()
        {
            // Directional Light 使用 GUI 模式绘制，大小不受 Camera 到它距离的影响
            const int rayCount = 8;

            using (new Gizmos.ColorScope(Color))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(transform)))
            {
                float guiScale = Gizmos.GUIScaleAtOrigin;
                float radius = guiScale * 0.15f;
                float rayLength = guiScale * 0.6f;

                Gizmos.DrawWireDisc(Vector3.Zero, Vector3.UnitZ, radius);

                for (int i = 0; i < rayCount; i++)
                {
                    float radians = MathF.PI * 2 / rayCount * i;
                    Vector3 pos = new Vector3(MathF.Sin(radians) * radius, MathF.Cos(radians) * radius, 0);
                    Gizmos.DrawLine(pos, pos + Vector3.UnitZ * rayLength);
                }
            }
        }

        private void DrawPointLightGizmos()
        {
            using (new Gizmos.ColorScope(Color))
            {
                Gizmos.DrawWireSphere(transform.Position, AttenuationRadius);
            }
        }

        private void DrawSpotLightGizmos()
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

            using (new Gizmos.ColorScope(Color))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(transform)))
            {
                drawCone(AttenuationRadius, float.DegreesToRadians(SpotInnerConeAngle), false);
                drawCone(AttenuationRadius, float.DegreesToRadians(SpotOuterConeAngle), true);
            }
        }

        [NativeMethod]
        private static partial nint New();
    }
}
