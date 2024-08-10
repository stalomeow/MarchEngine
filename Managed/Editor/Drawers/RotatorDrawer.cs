using DX12Demo.Core;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace DX12Demo.Editor.Drawers
{
    internal class RotatorDrawer : IPropertyDrawerFor<Rotator>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            var value = property.GetValue<Rotator>(target);
            Vector3 eulerAngles = value.EulerAngles;

            if (EditorGUI.Vector3Field(label, tooltip, ref eulerAngles))
            {
                value.EulerAngles = eulerAngles;
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
