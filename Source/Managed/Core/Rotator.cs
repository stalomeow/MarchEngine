using Newtonsoft.Json;
using System.Numerics;
using System.Runtime.CompilerServices;

namespace March.Core
{
    [JsonObject(MemberSerialization.OptIn)]
    internal struct Rotator
    {
        [JsonProperty] private Quaternion m_Quaternion;
        [JsonProperty] private Vector3 m_EulerAngles;

        public Rotator(Quaternion quaternion)
        {
            Value = quaternion;
        }

        public Quaternion Value
        {
            readonly get => m_Quaternion;
            set
            {
                m_Quaternion = value;
                m_EulerAngles = ToEulerAngles(value);
            }
        }

        public Vector3 EulerAngles
        {
            readonly get => m_EulerAngles;
            set
            {
                m_Quaternion = Quaternion.CreateFromYawPitchRoll(float.DegreesToRadians(value.Y),
                    float.DegreesToRadians(value.X), float.DegreesToRadians(value.Z));
                m_EulerAngles = value;
            }
        }

        public static Rotator Identity => new(Quaternion.Identity);

        private static Vector3 ToEulerAngles(Quaternion q)
        {
            Matrix4x4 m = Matrix4x4.Transpose(Matrix4x4.CreateFromQuaternion(q));
            Unsafe.SkipInit(out Vector3 euler);

            // YXZ Order https://www.geometrictools.com/Documentation/EulerAngles.pdf

            if (m[1, 2] < 1)
            {
                if (m[1, 2] > -1)
                {
                    euler.X = MathF.Asin(-m[1, 2]);
                    euler.Y = MathF.Atan2(m[0, 2], m[2, 2]);
                    euler.Z = MathF.Atan2(m[1, 0], m[1, 1]);
                }
                else // m[1, 2] == -1
                {
                    // Not a unique solution: euler.Z - euler.Y = MathF.Atan2(-m[0, 1], m[0, 0])
                    euler.X = MathF.PI / 2;
                    euler.Y = -MathF.Atan2(-m[0, 1], m[0, 0]);
                    euler.Z = 0;
                }
            }
            else // m[1, 2] == +1
            {
                // Not a unique solution: euler.Z + euler.Y = MathF.Atan2(-m[0, 1], m[0, 0])
                euler.X = -MathF.PI / 2;
                euler.Y = MathF.Atan2(-m[0, 1], m[0, 0]);
                euler.Z = 0;
            }

            return AdjustEulerAngles(euler);
        }

        public static Vector3 AdjustEulerAngles(Vector3 euler)
        {
            euler.X = float.RadiansToDegrees(euler.X);
            euler.Y = float.RadiansToDegrees(euler.Y);
            euler.Z = float.RadiansToDegrees(euler.Z);

            if (euler.Y < 0) euler.Y += 360;
            if (euler.Z < 0) euler.Z += 360;

            if (euler.Y >= 180 && euler.Z >= 180)
            {
                euler.X = 180 - euler.X;
                euler.Y -= 180;
                euler.Z -= 180;
            }

            if (euler.X < 0) euler.X += 360;
            if (euler.X == float.NegativeZero) euler.X = 0;
            if (euler.Y == float.NegativeZero) euler.Y = 0;
            if (euler.Z == float.NegativeZero) euler.Z = 0;

            return euler;
        }
    }
}
