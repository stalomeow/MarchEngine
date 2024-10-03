using March.Core.Serialization;

namespace March.Editor.Drawers
{
    internal class FloatDrawer : IPropertyDrawerFor<float>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            var value = property.GetValue<float>();
            var attr = property.GetAttribute<FloatDrawerAttribute>();
            bool changed;

            if (attr == null)
            {
                changed = EditorGUI.FloatField(label, tooltip, ref value);
            }
            else if (attr.Slider)
            {
                changed = EditorGUI.FloatSliderField(label, tooltip, ref value, attr.Min, attr.Max);
            }
            else
            {
                changed = EditorGUI.FloatField(label, tooltip, ref value, attr.DragSpeed, attr.Min, attr.Max);
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
