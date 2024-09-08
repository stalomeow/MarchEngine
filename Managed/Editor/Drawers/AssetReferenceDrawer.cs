using DX12Demo.Core;

namespace DX12Demo.Editor.Drawers
{
    internal class AssetReferenceDrawer<T> : IPropertyDrawerFor<AssetReference<T>> where T : EngineObject?
    {
        public bool Draw(string label, string tooltip, in EditorProperty property)
        {
            var value = property.GetValue<AssetReference<T>>();

            if (EditorGUI.AssetReferenceField(label, tooltip, ref value))
            {
                property.SetValue(value);
                return true;
            }

            return false;
        }
    }
}
