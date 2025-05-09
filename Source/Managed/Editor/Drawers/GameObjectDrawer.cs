using March.Core;
using March.Core.Diagnostics;
using March.Core.Pool;
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
            changed |= DrawComponent(0, Target.transform, out _);

            using var componentsToRemove = ListPool<Component>.Get();

            for (int i = 0; i < Target.m_Components.Count; i++)
            {
                var component = Target.m_Components[i];
                changed |= DrawComponent(i + 1, component, out bool shouldRemove);

                if (shouldRemove)
                {
                    componentsToRemove.Value.Add(component);
                }
            }

            foreach (var component in componentsToRemove.Value)
            {
                Target.DestroyAndRemoveComponent(component);
                changed = true;
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

        private static bool DrawComponent(int index, Component component, out bool shouldRemove)
        {
            Type componentType = component.GetType();

            if (PersistentManager.ResolveJsonContract(componentType) is not JsonObjectContract componentContract)
            {
                Log.Message(LogLevel.Error, "Failed to resolve json object contract", $"{componentType}");
                shouldRemove = true;
                return false;
            }

            using (new EditorGUI.IDScope(index))
            {
                bool isChanged = false;
                bool isEnabled = component.IsEnabled;
                var attr = componentType.GetCustomAttribute<CustomComponentAttribute>();

                EditorGUI.CursorPosX -= EditorGUI.CollapsingHeaderOuterExtend;

                using (new EditorGUI.DisabledScope(attr?.DisableCheckbox ?? false))
                {
                    if (EditorGUI.Checkbox("##ComponentEnabled", string.Empty, ref isEnabled))
                    {
                        isChanged = true;
                        component.IsEnabled = isEnabled;
                    }
                }

                EditorGUI.SameLine();

                bool open;

                if (attr?.HideRemoveButton ?? false)
                {
                    open = EditorGUI.CollapsingHeader(componentType.Name, defaultOpen: true);
                    shouldRemove = false;
                }
                else
                {
                    bool visible = true;
                    open = EditorGUI.CollapsingHeader(componentType.Name, ref visible, defaultOpen: true);
                    shouldRemove = !visible;
                }

                if (open)
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
                    GameObject? go = Selection.FirstOrDefault<GameObject>();
                    go?.AddComponent(componentType);
                }, enabled: (ref object? arg) => Selection.Any<GameObject>());
            }

            s_AddComponentPopupTypeCacheVersion = TypeCache.Version;
        }
    }
}
