using March.Core;

namespace March.Editor.Drawers
{
    internal class EngineObjectDrawer : IPropertyDrawerFor<EngineObject?>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            Type? type = property.PropertyType;

            if (type == null)
            {
                EditorGUI.LabelField(label, tooltip, "Can not determine property type");
                return false;
            }

            EngineObject? value = property.GetValue<EngineObject>();

            if (EditorGUI.EngineObjectField(label, tooltip, type, ref value))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
