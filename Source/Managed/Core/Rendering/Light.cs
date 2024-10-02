using March.Core.Binding;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Rendering
{
    public enum LightType : int
    {
        Directional = 0, // 平行光
        Point = 1,       // 点光源
        Spot = 2,        // 聚光灯
    };

    public partial class Light : Component
    {
        public Light() : base(Light_New()) { }

        protected override void Dispose(bool disposing)
        {
            Light_Delete(NativePtr);
            base.Dispose(disposing);
        }

        [JsonProperty]
        [Tooltip("The type of light.")]
        public LightType Type
        {
            get => Light_GetType(NativePtr);
            set => Light_SetType(NativePtr, value);
        }

        [JsonProperty]
        public Color Color
        {
            get => Light_GetColor(NativePtr);
            set => Light_SetColor(NativePtr, value);
        }

        [JsonProperty]
        [Tooltip("The range of the light's falloff.")]
        [Vector2Drawer(XNotGreaterThanY = true, Min = 0.1f)]
        public Vector2 FalloffRange
        {
            get => Light_GetFalloffRange(NativePtr);
            set => Light_SetFalloffRange(NativePtr, value);
        }

        [JsonProperty]
        [FloatDrawer(Min = 0.1f)]
        public float SpotPower
        {
            get => Light_GetSpotPower(NativePtr);
            set => Light_SetSpotPower(NativePtr, value);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint Light_New();

        [NativeFunction]
        private static partial void Light_Delete(nint self);

        [NativeFunction]
        private static partial LightType Light_GetType(nint self);

        [NativeFunction]
        private static partial void Light_SetType(nint self, LightType value);

        [NativeFunction]
        private static partial Color Light_GetColor(nint self);

        [NativeFunction]
        private static partial void Light_SetColor(nint self, Color value);

        [NativeFunction]
        private static partial Vector2 Light_GetFalloffRange(nint self);

        [NativeFunction]
        private static partial void Light_SetFalloffRange(nint self, Vector2 value);

        [NativeFunction]
        private static partial float Light_GetSpotPower(nint self);

        [NativeFunction]
        private static partial void Light_SetSpotPower(nint self, float value);

        #endregion
    }
}
