using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class FloatDrawer : IPropertyDrawerFor<float>
    {
        public bool Draw(string label, object target, JsonProperty property)
        {
            var value = property.GetValue<float>(target);

            if (EditorGUI.FloatField(label, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
