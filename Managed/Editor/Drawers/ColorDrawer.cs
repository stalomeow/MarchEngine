using DX12Demo.Core;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class ColorDrawer : IPropertyDrawerFor<Color>
    {
        public bool Draw(string label, object target, JsonProperty property)
        {
            var value = property.GetValue<Color>(target);

            if (EditorGUI.ColorField(label, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
