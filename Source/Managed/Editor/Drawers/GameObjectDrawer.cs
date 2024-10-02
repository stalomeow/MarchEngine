using March.Core;
using March.Core.Serialization;
using Newtonsoft.Json.Serialization;

namespace March.Editor.Drawers
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

            changed |= EditorGUI.PropertyField("##GameObjectIsActive", string.Empty, contract.GetEditorProperty(Target, "m_IsActive"));
            EditorGUI.SameLine();
            EditorGUI.SetNextItemWidth(EditorGUI.ContentRegionAvailable.X);
            changed |= EditorGUI.PropertyField("##GameObjectName", string.Empty, contract.GetEditorProperty(Target, "Name"));

            EditorGUI.SeparatorText("Components");
            changed |= DrawComponent(0, Target.transform);

            for (int i = 0; i < Target.m_Components.Count; i++)
            {
                changed |= DrawComponent(i + 1, Target.m_Components[i]);
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
                s_AddComponentPopup.AddMenuItem(displayName, callback: (ref object? arg) =>
                {
                    if (Selection.Active is GameObject go)
                    {
                        go.AddComponent(componentType);
                    }
                }, enabled: (ref object? arg) => Selection.Active is GameObject);
            }

            s_AddComponentPopupTypeCacheVersion = TypeCache.Version;
        }
    }
}
