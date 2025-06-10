using March.Core;
using March.Core.Interop;

namespace March.Editor.Drawers
{
    internal class MarchObjectDrawer : IPropertyDrawerFor<MarchObject?>
    {
        public bool Draw(StringLike label, StringLike tooltip, in EditorProperty property)
        {
            Type? type = property.PropertyType;

            if (type == null)
            {
                EditorGUI.LabelField(label, tooltip, "Can not determine property type");
                return false;
            }

            MarchObject? value = property.GetValue<MarchObject>();

            if (EditorGUI.MarchObjectField(label, tooltip, type, ref value))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
