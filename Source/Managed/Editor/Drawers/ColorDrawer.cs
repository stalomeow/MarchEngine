using March.Core;
using March.Core.Interop;

namespace March.Editor.Drawers
{
    internal class ColorDrawer : IPropertyDrawerFor<Color>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            var value = property.GetValue<Color>();

            if (EditorGUI.ColorField(label, tooltip, ref value))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
