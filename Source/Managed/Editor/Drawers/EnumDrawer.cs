using March.Core.Interop;

namespace March.Editor.Drawers
{
    internal class EnumDrawer : IPropertyDrawerFor<Enum>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            bool changed = false;
            Enum? value = property.GetValue<Enum>();

            if (value == null)
            {
                if (property.PropertyType == null)
                {
                    EditorGUI.LabelField(label, tooltip, "Can not determine enum type");
                    return false;
                }

                TypeCache.GetEnumData(property.PropertyType, out _, out ReadOnlySpan<Enum> values);

                if (values.Length <= 0)
                {
                    EditorGUI.LabelField(label, tooltip, "No enum values");
                    return false;
                }

                value = values[0];
                property.SetValue(value);
                changed = true;
            }

            if (EditorGUI.EnumField(label, tooltip, ref value))
            {
                property.SetValue(value);
                changed = true;
            }

            return changed;
        }
    }
}
