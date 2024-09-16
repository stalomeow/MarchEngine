using Newtonsoft.Json;
using System.Numerics;

namespace March.Core.Test
{
    public sealed class RotateMe : Component
    {
        public enum RotateAxis
        {
            XAxis,
            YAxis,
            ZAxis
        }

        [JsonProperty] public float Speed = 20.0f;
        [JsonProperty] public RotateAxis Axis = RotateAxis.XAxis;

        protected override void OnUpdate()
        {
            base.OnUpdate();

            Quaternion rot = Quaternion.CreateFromAxisAngle(Axis switch
            {
                RotateAxis.XAxis => Vector3.UnitX,
                RotateAxis.YAxis => Vector3.UnitY,
                RotateAxis.ZAxis => Vector3.UnitZ,
                _ => throw new ArgumentOutOfRangeException()
            }, float.DegreesToRadians(Time.Delta * Speed));
            MountingObject.Rotation = rot * MountingObject.Rotation;
        }
    }
}
