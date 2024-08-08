using DX12Demo.Core.Binding;
using Newtonsoft.Json;
using System.Numerics;

namespace DX12Demo.Core.Rendering
{
    public enum LightType : int
    {
        Directional = 0, // 平行光
        Point = 1,       // 点光源
        Spot = 2,        // 聚光灯
    };

    public partial class Light : Component
    {
        private nint m_Light;

        public Light()
        {
            m_Light = Light_New();
        }

        ~Light()
        {
            Dispose();
        }

        [JsonProperty] public LightType Type
        {
            get => Light_GetType(m_Light);
            set => Light_SetType(m_Light, value);
        }

        [JsonProperty] public Color Color
        {
            get => Light_GetColor(m_Light);
            set => Light_SetColor(m_Light, value);
        }

        [JsonProperty] public Vector2 FalloffRange
        {
            get => Light_GetFalloffRange(m_Light);
            set => Light_SetFalloffRange(m_Light, value);
        }

        [JsonProperty] public float SpotPower
        {
            get => Light_GetSpotPower(m_Light);
            set => Light_SetSpotPower(m_Light, value);
        }

        private void Dispose()
        {
            if (m_Light == nint.Zero)
            {
                return;
            }

            Light_Delete(m_Light);
            m_Light = nint.Zero;
        }

        protected override void OnMount()
        {
            base.OnMount();
            RenderPipeline.AddLight(m_Light);
        }

        protected override void OnUnmount()
        {
            RenderPipeline.RemoveLight(m_Light);
            Dispose();
            GC.SuppressFinalize(this);
            base.OnUnmount();
        }

        protected override void OnEnable()
        {
            base.OnEnable();
            Light_SetIsActive(m_Light, true);
        }

        protected override void OnDisable()
        {
            Light_SetIsActive(m_Light, false);
            base.OnDisable();
        }

        protected override void OnUpdate()
        {
            base.OnUpdate();

            Light_SetPosition(m_Light, MountingObject.Position);
            Light_SetRotation(m_Light, MountingObject.Rotation);
        }

        [NativeFunction]
        private static partial nint Light_New();

        [NativeFunction]
        private static partial void Light_Delete(nint self);

        [NativeFunction]
        private static partial void Light_SetPosition(nint self, Vector3 value);

        [NativeFunction]
        private static partial void Light_SetRotation(nint self, Quaternion value);

        [NativeFunction]
        private static partial void Light_SetIsActive(nint self, bool value);

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
    }
}
