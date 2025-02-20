using System.Numerics;

namespace March.Core
{
    public static class MathUtility
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="q"></param>
        /// <param name="angle">单位是弧度</param>
        /// <param name="axis"></param>
        public static void ToAngleAxis(this Quaternion q, out float angle, out Vector3 axis)
        {
            angle = MathF.Acos(q.W) * 2.0f;

            axis = new Vector3(q.X, q.Y, q.Z);
            float lengthSquared = 1 - q.W * q.W;

            if (lengthSquared > 0)
            {
                axis /= MathF.Sqrt(lengthSquared); // normalize
            }
            else
            {
                // 旋转角是 0 度，任意返回一个单位向量
                axis = Vector3.UnitX;
            }
        }
    }
}
