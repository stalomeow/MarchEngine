using DX12Demo.Core;
using Newtonsoft.Json.Serialization;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor.Windows
{
    internal static class InspectorWindow
    {
        private static readonly PopupMenu s_ComponentPopup = new("InspectorAddComponentPopup");
        private static int s_ComponentPopupTypeCacheVersion;

        private static readonly DrawerCache<IComponentDrawer> s_ComponentDrawerCache = new(typeof(IComponentDrawerFor<>));

        static InspectorWindow()
        {
            RebuildComponentPopup();
        }

        private static void RebuildComponentPopup()
        {
            s_ComponentPopup.Clear();
            s_ComponentPopup.AddSeparatorText("", "Add Component Menu");

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
                s_ComponentPopup.AddMenuItem(displayName, callback: () =>
                {
                    if (Selection.Active is GameObject go)
                    {
                        go.AddComponent(componentType);
                    }
                }, enabled: () => Selection.Active is GameObject);
            }

            s_ComponentPopupTypeCacheVersion = TypeCache.Version;
        }

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            switch (Selection.Active)
            {
                case GameObject go:
                    DrawGameObjectInspector(go);
                    break;
            }
        }

        private static bool DrawGameObjectInspector(GameObject go)
        {
            var goContract = (JsonObjectContract)PersistentManager.ResolveJsonContract(typeof(GameObject));
            var changed = false;

            changed |= EditorGUI.PropertyField("##GameObjectIsActive", string.Empty, go, goContract.Properties["m_IsActive"]);
            EditorGUI.SameLine();
            EditorGUI.SetNextItemWidth(EditorGUI.GetContentRegionAvailable().X);
            changed |= EditorGUI.PropertyField("##GameObjectName", string.Empty, go, goContract.Properties["Name"]);

            EditorGUI.SeparatorText("Transform");

            changed |= EditorGUI.PropertyField(go, goContract.Properties["Position"]);
            changed |= EditorGUI.PropertyField(go, goContract.Properties["Rotation"]);
            changed |= EditorGUI.PropertyField(go, goContract.Properties["Scale"]);

            EditorGUI.SeparatorText("Components");

            for (int i = 0; i < go.m_Components.Count; i++)
            {
                Component component = go.m_Components[i];
                Type componentType = component.GetType();

                if (PersistentManager.ResolveJsonContract(componentType) is not JsonObjectContract componentContract)
                {
                    Debug.LogError($"Failed to resolve json object contract for {componentType}.");
                    continue;
                }

                if (!EditorGUI.CollapsingHeader(componentType.Name, defaultOpen: true))
                {
                    continue;
                }

                using (new EditorGUI.IDScope(i))
                {
                    if (s_ComponentDrawerCache.TryGet(componentType, out IComponentDrawer? drawer))
                    {
                        changed |= drawer.Draw(component, componentContract);
                    }
                    else
                    {
                        changed |= DefaultDrawComponent(component, componentContract);
                    }
                }

                EditorGUI.Space();
            }

            EditorGUI.Space();

            if (EditorGUI.CenterButton("Add Component", 220.0f))
            {
                s_ComponentPopup.Open();
            }

            if (s_ComponentPopupTypeCacheVersion != TypeCache.Version)
            {
                RebuildComponentPopup();
            }

            s_ComponentPopup.Draw();

            return changed;
        }

        private static bool DefaultDrawComponent(Component component, JsonObjectContract contract)
        {
            bool changed = false;

            foreach (var property in contract.Properties)
            {
                if (property.Ignored)
                {
                    continue;
                }

                changed |= EditorGUI.PropertyField(component, property);
            }

            return changed;
        }
    }
}
