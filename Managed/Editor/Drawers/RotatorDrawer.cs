using DX12Demo.Core;
using System.Numerics;

namespace DX12Demo.Editor.Drawers
{
    internal class RotatorDrawer : IPropertyDrawerFor<Rotator>
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            var value = property.GetValue<Rotator>();
            Vector3 eulerAngles = value.EulerAngles;

            if (EditorGUI.Vector3Field(label, tooltip, ref eulerAngles))
            {
                value.EulerAngles = eulerAngles;
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
