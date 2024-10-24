using March.Core.Binding;
using March.Core.IconFonts;
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

        protected override void DisposeNative()
        {
            Light_Delete(NativePtr);
            base.DisposeNative();
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

        protected override void OnDrawGizmos(bool isSelected)
        {
            base.OnDrawGizmos(isSelected);

            if (!isSelected)
            {
                return;
            }

            using (new GizmosGUI.ColorScope(Color))
            {
                Vector3 position = gameObject.transform.Position;

                if (Type == LightType.Point)
                {
                    Gizmos.DrawWireSphere(position, FalloffRange.Y);
                }
                else if (Type == LightType.Spot)
                {
                    //// 当强度为 0.01 时的边界
                    //float halfAngle = MathF.Acos(MathF.Pow(10, -2.0f / SpotPower));

                    //Vector3 forward = gameObject.transform.Forward;
                    //Vector3 right = gameObject.transform.Right;
                    //Vector3 up = gameObject.transform.Up;

                    //Vector3 forwardDir = forward * FalloffRange.Y;
                    //Vector3 posForward = position + forwardDir;
                    //Vector3 posUp = position + Vector3.Transform(forwardDir, Quaternion.CreateFromAxisAngle(right, -halfAngle));
                    //Vector3 posDown = position + Vector3.Transform(forwardDir, Quaternion.CreateFromAxisAngle(right, halfAngle));
                    //Vector3 posLeft = position + Vector3.Transform(forwardDir, Quaternion.CreateFromAxisAngle(up, -halfAngle));
                    //Vector3 posRight = position + Vector3.Transform(forwardDir, Quaternion.CreateFromAxisAngle(up, halfAngle));

                    //Gizmos.DrawLine(position, posUp, Color);
                    //Gizmos.DrawLine(position, posDown, Color);
                    //Gizmos.DrawLine(position, posLeft, Color);
                    //Gizmos.DrawLine(position, posRight, Color);

                    //Vector3 circleCenter = position + MathF.Cos(halfAngle) * forwardDir;
                    //float circleRadius = FalloffRange.Y * MathF.Sin(halfAngle);
                    //Gizmos.DrawWireCircle(circleCenter, forward, circleRadius, Color);

                    //float halfDeg = float.RadiansToDegrees(halfAngle);
                    //Gizmos.DrawWireArc(position, right, forward, halfDeg, FalloffRange.Y, Color);
                    //Gizmos.DrawWireArc(position, right, forward, -halfDeg, FalloffRange.Y, Color);
                    //Gizmos.DrawWireArc(position, up, forward, halfDeg, FalloffRange.Y, Color);
                    //Gizmos.DrawWireArc(position, up, forward, -halfDeg, FalloffRange.Y, Color);
                }
            }
        }

        protected override void OnDrawGizmosGUI(bool isSelected)
        {
            base.OnDrawGizmosGUI(isSelected);

            using (new GizmosGUI.ColorScope(Color))
            {
                GizmosGUI.DrawText(gameObject.transform.Position, Type switch
                {
                    LightType.Directional => FontAwesome6.Sun,
                    LightType.Point => FontAwesome6.Lightbulb,
                    LightType.Spot => FontAwesome6.LandMineOn,
                    _ => throw new NotSupportedException()
                });

                //if (isSelected && Type == LightType.Directional)
                //{
                //    const float radius = 0.4f;

                //    Vector3 forward = gameObject.transform.Forward;
                //    Vector3 up = gameObject.transform.Up;
                //    Vector3 right = gameObject.transform.Right;
                //    Gizmos.DrawWireCircle(position, forward, radius, Color);

                //    const int numSegments = 8;

                //    for (int i = 0; i < numSegments; i++)
                //    {
                //        float rad = MathF.PI * 2 / numSegments * i;
                //        Vector3 pos = position + radius * (MathF.Sin(rad) * right + MathF.Cos(rad) * up);
                //        Gizmos.DrawLine(pos, pos + forward * 0.8f, Color);
                //    }
                //}
            }
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
