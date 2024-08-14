using DX12Demo.Core;
using DX12Demo.Core.Serialization;
using Newtonsoft.Json.Serialization;

namespace DX12Demo.Editor.Drawers
{
    internal class GameObjectDrawer : InspectorDrawerFor<GameObject>
    {
        private static readonly DrawerCache<IComponentDrawer> s_ComponentDrawerCache = new(typeof(IComponentDrawerFor<>));
        private static readonly PopupMenu s_AddComponentPopup = new("GameObjectInspectorAddComponentPopup");
        private static int? s_AddComponentPopupTypeCacheVersion;

        public override void Draw()
        {
            var contract = (JsonObjectContract)PersistentManager.ResolveJsonContract(typeof(GameObject));
            var changed = false;

            changed |= EditorGUI.PropertyField("##GameObjectIsActive", string.Empty, Target, contract.Properties["m_IsActive"]);
            EditorGUI.SameLine();
            EditorGUI.SetNextItemWidth(EditorGUI.GetContentRegionAvailable().X);
            changed |= EditorGUI.PropertyField("##GameObjectName", string.Empty, Target, contract.Properties["Name"]);

            EditorGUI.SeparatorText("Transform");

            changed |= EditorGUI.PropertyField(Target, contract.Properties["Position"]);
            changed |= EditorGUI.PropertyField(Target, contract.Properties["Rotation"]);
            changed |= EditorGUI.PropertyField(Target, contract.Properties["Scale"]);

            EditorGUI.SeparatorText("Components");

            for (int i = 0; i < Target.m_Components.Count; i++)
            {
                changed |= DrawComponent(i, Target.m_Components[i]);
            }

            EditorGUI.Space();

            if (EditorGUI.CenterButton("Add Component", 220.0f))
            {
                s_AddComponentPopup.Open();
            }

            if (s_AddComponentPopupTypeCacheVersion != TypeCache.Version)
            {
                RebuildAddComponentPopup();
            }

            s_AddComponentPopup.Draw();

            if (changed)
            {
                // TODO save changes
            }
        }

        private static bool DrawComponent(int index, Component component)
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
                if (s_ComponentDrawerCache.TryGetSharedInstance(componentType, out IComponentDrawer? drawer))
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

        private static void RebuildAddComponentPopup()
        {
            s_AddComponentPopup.Clear();
            s_AddComponentPopup.AddSeparatorText("", "Add Component Menu");

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
                s_AddComponentPopup.AddMenuItem(displayName, callback: () =>
                {
                    if (Selection.Active is GameObject go)
                    {
                        go.AddComponent(componentType);
                    }
                }, enabled: () => Selection.Active is GameObject);
            }

            s_AddComponentPopupTypeCacheVersion = TypeCache.Version;
        }
    }
}
