using DX12Demo.Core;
using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class GameObjectDrawer : IInspectorDrawerFor<GameObject>
    {
        private readonly DrawerCache<IComponentDrawer> m_ComponentDrawerCache = new(typeof(IComponentDrawerFor<>));
        private readonly PopupMenu m_AddComponentPopup = new("GameObjectInspectorAddComponentPopup");
        private int? m_AddComponentPopupTypeCacheVersion;

        public void Draw(GameObject target)
        {
            var contract = (JsonObjectContract)PersistentManager.ResolveJsonContract(typeof(GameObject));
            var changed = false;

            changed |= EditorGUI.PropertyField("##GameObjectIsActive", string.Empty, target, contract.Properties["m_IsActive"]);
            EditorGUI.SameLine();
            EditorGUI.SetNextItemWidth(EditorGUI.GetContentRegionAvailable().X);
            changed |= EditorGUI.PropertyField("##GameObjectName", string.Empty, target, contract.Properties["Name"]);

            EditorGUI.SeparatorText("Transform");

            changed |= EditorGUI.PropertyField(target, contract.Properties["Position"]);
            changed |= EditorGUI.PropertyField(target, contract.Properties["Rotation"]);
            changed |= EditorGUI.PropertyField(target, contract.Properties["Scale"]);

            EditorGUI.SeparatorText("Components");

            for (int i = 0; i < target.m_Components.Count; i++)
            {
                changed |= DrawComponent(i, target.m_Components[i]);
            }

            EditorGUI.Space();

            if (EditorGUI.CenterButton("Add Component", 220.0f))
            {
                m_AddComponentPopup.Open();
            }

            if (m_AddComponentPopupTypeCacheVersion != TypeCache.Version)
            {
                RebuildAddComponentPopup();
            }

            m_AddComponentPopup.Draw();

            if (changed)
            {
                // TODO save changes
            }
        }

        private bool DrawComponent(int index, Component component)
        {
            Type componentType = component.GetType();

            if (PersistentManager.ResolveJsonContract(componentType) is not JsonObjectContract componentContract)
            {
                Debug.LogError($"Failed to resolve json object contract for {componentType}.");
                return false;
            }

            if (!EditorGUI.CollapsingHeader(componentType.Name, defaultOpen: true))
            {
                return false;
            }

            var changed = false;

            using (new EditorGUI.IDScope(index))
            {
                if (m_ComponentDrawerCache.TryGet(componentType, out IComponentDrawer? drawer))
                {
                    changed |= drawer.Draw(component, componentContract);
                }
                else
                {
                    changed |= EditorGUI.ObjectPropertyFields(component, componentContract);
                }
            }

            EditorGUI.Space();
            return changed;
        }

        private void RebuildAddComponentPopup()
        {
            m_AddComponentPopup.Clear();
            m_AddComponentPopup.AddSeparatorText("", "Add Component Menu");

            foreach (Type type in TypeCache.GetTypesDerivedFrom<Component>())
            {
                if (type.IsAbstract || type.IsGenericType)
                {
                    continue;
                }

                string displayName = type.Name;
                if (type.Namespace != null)
                {
                    displayName = $"{type.Namespace}/{type.Name}";
                }

                Type componentType = type;
                m_AddComponentPopup.AddMenuItem(displayName, callback: () =>
                {
                    if (Selection.Active is GameObject go)
                    {
                        go.AddComponent(componentType);
                    }
                }, enabled: () => Selection.Active is GameObject);
            }

            m_AddComponentPopupTypeCacheVersion = TypeCache.Version;
        }
    }
}
