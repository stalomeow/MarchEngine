using March.Core;
using March.Core.Serialization;
using Newtonsoft.Json.Serialization;
using System.Reflection;

namespace March.Editor.Drawers
{
    internal class GameObjectDrawer : InspectorDrawerFor<GameObject>
    {
        private static readonly DrawerCache<IComponentDrawer> s_ComponentDrawerCache = new(typeof(IComponentDrawerFor<>));
        private static readonly GenericMenu s_AddComponentPopup = new("GameObjectInspectorAddComponentPopup");
        private static int? s_AddComponentPopupTypeCacheVersion;

        public override void Draw()
        {
            var contract = (JsonObjectContract)PersistentManager.ResolveJsonContract(typeof(GameObject));
            var changed = false;

            bool isActiveSelf = Target.IsActiveSelf;
            EditorGUI.CursorPosX -= EditorGUI.CollapsingHeaderOuterExtend;
            if (EditorGUI.Checkbox("##GameObjectIsActive", string.Empty, ref isActiveSelf))
            {
                Target.IsActiveSelf = isActiveSelf;
                changed = true;
            }

            EditorGUI.SameLine();

            EditorGUI.CursorPosX -= EditorGUI.CollapsingHeaderOuterExtend;
            EditorGUI.SetNextItemWidth(EditorGUI.ContentRegionAvailable.X + EditorGUI.CollapsingHeaderOuterExtend);
            changed |= EditorGUI.PropertyField("##GameObjectName", string.Empty, contract.GetEditorProperty(Target, "m_Name"));

            EditorGUI.SeparatorText("Components");
            changed |= DrawComponent(0, Target.transform);

            for (int i = 0; i < Target.m_Components.Count; i++)
            {
                changed |= DrawComponent(i + 1, Target.m_Components[i]);
            }

            EditorGUI.Space();

            if (EditorGUI.CenterButton("Add Component", 220.0f))
            {
                s_AddComponentPopup.OpenPopup();
            }

            if (s_AddComponentPopupTypeCacheVersion != TypeCache.Version)
            {
                RebuildAddComponentPopup();
            }

            s_AddComponentPopup.DrawPopup();

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

            using (new EditorGUI.IDScope(index))
            {
                bool isChanged = false;
                bool isEnabled = component.IsEnabled;

                EditorGUI.CursorPosX -= EditorGUI.CollapsingHeaderOuterExtend;

                using (new EditorGUI.DisabledScope(componentType.GetCustomAttribute<DisableComponentEnabledCheckboxAttribute>() != null))
                {
                    if (EditorGUI.Checkbox("##ComponentEnabled", string.Empty, ref isEnabled))
                    {
                        isChanged = true;
                        component.IsEnabled = isEnabled;
                    }
                }

                EditorGUI.SameLine();

                if (EditorGUI.CollapsingHeader(componentType.Name, defaultOpen: true))
                {
                    if (s_ComponentDrawerCache.TryGetSharedInstance(componentType, out IComponentDrawer? drawer))
                    {
                        isChanged |= drawer.Draw(component, componentContract);
                    }
                    else
                    {
                        isChanged |= EditorGUI.ObjectPropertyFields(component, componentContract);
                    }

                    EditorGUI.Space();
                }

                return isChanged;
            }
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
