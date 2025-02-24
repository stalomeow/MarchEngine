using March.Core;
using March.Core.Interop;
using March.Core.Serialization;

namespace March.Editor.Drawers
{
    internal class ColorDrawer : IPropertyDrawerFor<Color>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = property.GetValue<Color>();
            var attr = property.GetAttribute<ColorDrawerAttribute>();
            bool changed;

            if (attr != null)
            {
                changed = EditorGUI.ColorField(label, tooltip, ref value, attr.Alpha, attr.HDR);
            }
            else
            {
                changed = EditorGUI.ColorField(label, tooltip, ref value);
            }

            if (changed)
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
