using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class BoolDrawer : IPropertyDrawerFor<bool>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            var value = property.GetValue<bool>(target);

            if (EditorGUI.Checkbox(label, tooltip, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
