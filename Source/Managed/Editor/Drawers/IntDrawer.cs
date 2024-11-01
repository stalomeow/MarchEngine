using March.Core.Interop;

namespace March.Editor.Drawers
{
    internal class Int8Drawer : IPropertyDrawerFor<sbyte>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = (int)property.GetValue<sbyte>();

            if (EditorGUI.IntField(label, tooltip, ref value, 1, sbyte.MinValue, sbyte.MaxValue))
            {
                property.SetValue((sbyte)value);
                return true;
            }

            return false;
        }
    }

    internal class UInt8Drawer : IPropertyDrawerFor<byte>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = (int)property.GetValue<byte>();

            if (EditorGUI.IntField(label, tooltip, ref value, 1, byte.MinValue, byte.MaxValue))
            {
                property.SetValue((byte)value);
                return true;
            }

            return false;
        }
    }

    internal class Int16Drawer : IPropertyDrawerFor<short>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = (int)property.GetValue<short>();

            if (EditorGUI.IntField(label, tooltip, ref value, 1, short.MinValue, short.MaxValue))
            {
                property.SetValue((short)value);
                return true;
            }

            return false;
        }
    }

    internal class UInt16Drawer : IPropertyDrawerFor<ushort>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = (int)property.GetValue<ushort>();

            if (EditorGUI.IntField(label, tooltip, ref value, 1, ushort.MinValue, ushort.MaxValue))
            {
                property.SetValue((ushort)value);
                return true;
            }

            return false;
        }
    }

    internal class Int32Drawer : IPropertyDrawerFor<int>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = property.GetValue<int>();

            if (EditorGUI.IntField(label, tooltip, ref value, 1, int.MinValue, int.MaxValue))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
