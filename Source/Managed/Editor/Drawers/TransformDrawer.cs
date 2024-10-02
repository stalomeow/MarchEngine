using March.Core;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace March.Editor.Drawers
{
    internal class TransformDrawer : IComponentDrawerFor<Transform>
    {
        public bool Draw(Transform component, JsonObjectContract contract)
        {
            bool changed = false;

            Vector3 localPosition = component.LocalPosition;
            Vector3 localEulerAngles = component.LocalEulerAngles;
            Vector3 localScale = component.LocalScale;

            if (EditorGUI.Vector3Field("Position", string.Empty, ref localPosition))
            {
                component.LocalPosition = localPosition;
                changed = true;
            }

            if (EditorGUI.Vector3Field("Rotation", string.Empty, ref localEulerAngles))
            {
                component.LocalEulerAngles = localEulerAngles;
                changed = true;
            }

            if (EditorGUI.Vector3Field("Scale", string.Empty, ref localScale))
            {
                component.LocalScale = localScale;
                changed = true;
            }

            return changed;
        }
    }
}
