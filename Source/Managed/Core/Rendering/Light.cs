using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;

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

        // 反序列化顺序（由声明的顺序决定）非常重要，因为一些属性会相互影响
        // 正确的顺序：Type -> Color -> UseColorTemperature -> ColorTemperature -> Unit -> Intensity

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

        [JsonProperty]
        [InspectorName("Angular Diameter")]
        [FloatDrawer(Min = 0.0f, Max = 90.0f)]
        [NativeProperty]
        public partial float AngularDiameter { get; set; } // in degrees

        [JsonProperty]
        [InspectorName("Depth Bias")]
        [NativeProperty]
        public partial int ShadowDepthBias { get; set; }

        [JsonProperty]
        [InspectorName("Slope Scaled Bias")]
        [NativeProperty]
        public partial float ShadowSlopeScaledDepthBias { get; set; }

        [JsonProperty]
        [InspectorName("Depth Bias Clamp")]
        [NativeProperty]
        public partial float ShadowDepthBiasClamp { get; set; }

        [JsonProperty]
        [InspectorName("Casts Shadow")]
        [Tooltip("Currently only Directional Light supports shadow and only one shadow map is supported.")]
        [NativeProperty]
        public partial bool IsCastingShadow { get; set; }

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

        [NativeMethod]
        private static partial nint New();
    }
}
