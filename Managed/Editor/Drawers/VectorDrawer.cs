using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace DX12Demo.Editor.Drawers
{
    internal class Vector2Drawer : IPropertyDrawerFor<Vector2>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            var value = property.GetValue<Vector2>(target);
            var attr = property.GetAttribute<Vector2DrawerAttribute>();
            bool changed;

            if (attr == null)
            {
                changed = EditorGUI.Vector2Field(label, tooltip, ref value);
            }
            else if (attr.XNotGreaterThanY)
            {
                changed = EditorGUI.FloatRangeField(label, tooltip, ref value.X, ref value.Y, attr.DragSpeed, attr.Min, attr.Max);
            }
            else
            {
                changed = EditorGUI.Vector2Field(label, tooltip, ref value, attr.DragSpeed, attr.Min, attr.Max);
            }

            if (changed)
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }

    internal class Vector3Drawer : IPropertyDrawerFor<Vector3>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            var value = property.GetValue<Vector3>(target);

            if (EditorGUI.Vector3Field(label, tooltip, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }

    internal class Vector4Drawer : IPropertyDrawerFor<Vector4>
    {
        public bool Draw(string label, string tooltip, object target, JsonProperty property)
        {
            var value = property.GetValue<Vector4>(target);

            if (EditorGUI.Vector4Field(label, tooltip, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
