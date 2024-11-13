using March.Core.IconFont;
using March.Core.Interop;
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
        public Light() : base(New()) { }

        protected override void DisposeNative()
        {
            Delete();
            base.DisposeNative();
        }

        [JsonProperty]
        [Tooltip("The type of light.")]
        [NativeProperty]
        public partial LightType Type { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial Color Color { get; set; }

        [JsonProperty]
        [Tooltip("The range of the light's falloff.")]
        [Vector2Drawer(XNotGreaterThanY = true, Min = 0.1f)]
        [NativeProperty]
        public partial Vector2 FalloffRange { get; set; }

        [JsonProperty]
        [FloatDrawer(Min = 0.1f)]
        [NativeProperty]
        public partial float SpotPower { get; set; }

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
                Gizmos.DrawWireSphere(transform.Position, FalloffRange.Y);
            }
        }

        private void DrawSpotLightGizmos()
        {
            using (new Gizmos.ColorScope(Color))
            using (new Gizmos.MatrixScope(GizmosUtility.GetLocalToWorldMatrixWithoutScale(transform)))
            {
                // 当强度为 0.01 时的边界
                float halfAngle = MathF.Acos(MathF.Pow(10, -2.0f / SpotPower));
                float sinHalfAngle = MathF.Sin(halfAngle) * FalloffRange.Y;
                float cosHalfAngle = MathF.Cos(halfAngle) * FalloffRange.Y;

                Gizmos.DrawLine(Vector3.Zero, new Vector3(0, sinHalfAngle, cosHalfAngle));
                Gizmos.DrawLine(Vector3.Zero, new Vector3(0, -sinHalfAngle, cosHalfAngle));
                Gizmos.DrawLine(Vector3.Zero, new Vector3(sinHalfAngle, 0, cosHalfAngle));
                Gizmos.DrawLine(Vector3.Zero, new Vector3(-sinHalfAngle, 0, cosHalfAngle));

                Gizmos.DrawWireDisc(new Vector3(0, 0, cosHalfAngle), Vector3.UnitZ, sinHalfAngle);

                Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitX, Vector3.UnitZ, halfAngle, FalloffRange.Y);
                Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitX, Vector3.UnitZ, -halfAngle, FalloffRange.Y);
                Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitY, Vector3.UnitZ, halfAngle, FalloffRange.Y);
                Gizmos.DrawWireArc(Vector3.Zero, Vector3.UnitY, Vector3.UnitZ, -halfAngle, FalloffRange.Y);
            }
        }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void Delete();
    }
}
