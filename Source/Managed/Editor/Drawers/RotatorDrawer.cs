using March.Core;
using System.Numerics;

namespace March.Editor.Drawers
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
