using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class StringDrawer : IPropertyDrawerFor<string>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            bool changed = false;
            string? value = property.GetValue<string>(target);

            if (value == null)
            {
                value = string.Empty;
                property.SetValue(target, value);
                changed = true;
            }

            if (EditorGUI.TextField(label, tooltip, ref value))
            {
                property.SetValue(target, value);
                changed = true;
            }

            return changed;
        }
    }
}
