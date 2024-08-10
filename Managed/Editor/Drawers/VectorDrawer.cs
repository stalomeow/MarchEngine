using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace DX12Demo.Editor.Drawers
{
    internal class Vector2Drawer : IPropertyDrawerFor<Vector2>
    {
        public bool Draw(string label, object target, JsonProperty property)
        {
            var value = property.GetValue<Vector2>(target);

            if (EditorGUI.Vector2Field(label, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }

    internal class Vector3Drawer : IPropertyDrawerFor<Vector3>
    {
        public bool Draw(string label, object target, JsonProperty property)
        {
            var value = property.GetValue<Vector3>(target);

            if (EditorGUI.Vector3Field(label, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }

    internal class Vector4Drawer : IPropertyDrawerFor<Vector4>
    {
        public bool Draw(string label, object target, JsonProperty property)
        {
            var value = property.GetValue<Vector4>(target);

            if (EditorGUI.Vector4Field(label, ref value))
            {
                property.SetValue(target, value);
                return true;
            }

            return false;
        }
    }
}
