using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class FloatDrawer : IPropertyDrawerFor<float>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            var value = property.GetValue<float>(target);
            var attr = property.GetAttribute<FloatDrawerAttribute>();
            bool changed;

            if (attr == null)
            {
                changed = EditorGUI.FloatField(label, tooltip, ref value);
            }
            else
            {
                changed = EditorGUI.FloatField(label, tooltip, ref value, attr.DragSpeed, attr.Min, attr.Max);
            }

            if (changed)
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
