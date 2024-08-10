using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class EnumDrawer : IPropertyDrawerFor<Enum>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            bool changed = false;
            Enum? value = property.GetValue<Enum>(target);

            if (value == null)
            {
                value = (Enum)Enum.ToObject(property.PropertyType!, 0);
                property.SetValue(target, value);
                changed = true;
            }

            if (EditorGUI.EnumField(label, tooltip, ref value))
            {
                property.SetValue(target, value);
                changed = true;
            }

            return changed;
        }
    }
}
