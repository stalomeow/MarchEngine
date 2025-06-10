using System.Numerics;

namespace March.Core
{
    public static class BinaryUtility
    {
        public static void Write(this BinaryWriter writer, in Vector2 value)
        {
            writer.Write(value.X);
            writer.Write(value.Y);
        }

        public static void Write(this BinaryWriter writer, in Vector3 value)
        {
            writer.Write(value.X);
            writer.Write(value.Y);
            writer.Write(value.Z);
        }

        public static void Write(this BinaryWriter writer, in Vector4 value)
        {
            writer.Write(value.X);
            writer.Write(value.Y);
            writer.Write(value.Z);
            writer.Write(value.W);
        }

        public static void Write(this BinaryWriter writer, in Quaternion value)
        {
            writer.Write(value.X);
            writer.Write(value.Y);
            writer.Write(value.Z);
            writer.Write(value.W);
        }

        public static void Write(this BinaryWriter writer, in Matrix4x4 value)
        {
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    writer.Write(value[i, j]);
                }
            }
        }

        public static void Write(this BinaryWriter writer, in Bounds bounds)
        {
            writer.Write(bounds.Center);
            writer.Write(bounds.Extents);
        }

        public static Vector2 ReadVector2(this BinaryReader reader)
        {
            float x = reader.ReadSingle();
            float y = reader.ReadSingle();
            return new Vector2(x, y);
        }

        public static Vector3 ReadVector3(this BinaryReader reader)
        {
            float x = reader.ReadSingle();
            float y = reader.ReadSingle();
            float z = reader.ReadSingle();
            return new Vector3(x, y, z);
        }

        public static Vector4 ReadVector4(this BinaryReader reader)
        {
            float x = reader.ReadSingle();
            float y = reader.ReadSingle();
            float z = reader.ReadSingle();
            float w = reader.ReadSingle();
            return new Vector4(x, y, z, w);
        }

        public static Quaternion ReadQuaternion(this BinaryReader reader)
        {
            float x = reader.ReadSingle();
            float y = reader.ReadSingle();
            float z = reader.ReadSingle();
            float w = reader.ReadSingle();
            return new Quaternion(x, y, z, w);
        }

        public static Matrix4x4 ReadMatrix4x4(this BinaryReader reader)
        {
            Matrix4x4 m = new();

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    m[i, j] = reader.ReadSingle();
                }
            }

            return m;
        }

        public static Bounds ReadBounds(this BinaryReader reader)
        {
            Vector3 center = reader.ReadVector3();
            Vector3 extents = reader.ReadVector3();
            return new Bounds(center, extents);
        }
    }
}
