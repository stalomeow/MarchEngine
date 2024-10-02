using March.Core;
using March.Core.Rendering;
using System.Runtime.InteropServices;

namespace March.Editor.Windows
{
    internal static class HierarchyWindow
    {
        private static readonly PopupMenu s_ContextMenu = new("HierarchyContextMenu");

        static HierarchyWindow()
        {
            s_ContextMenu.AddMenuItem("Create/Primitive/Cube", (ref object? arg) =>
            {
                var go = new GameObject() { Name = "Cube" };
                SceneManager.CurrentScene.AddGameObject(Selection.Active as GameObject, go);

                var mr = go.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Cube;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Primitive/Sphere", (ref object? arg) =>
            {
                var go = new GameObject() { Name = "Sphere" };
                SceneManager.CurrentScene.AddGameObject(Selection.Active as GameObject, go);

                var mr = go.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Sphere;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Light", (ref object? arg) =>
            {
                var go = new GameObject() { Name = "Directional Light" };
                SceneManager.CurrentScene.AddGameObject(Selection.Active as GameObject, go);

                go.AddComponent<Light>();

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Empty", (ref object? arg) =>
            {
                var go = new GameObject();
                SceneManager.CurrentScene.AddGameObject(Selection.Active as GameObject, go);
                Selection.Active = go;
            });
        }

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            Scene scene = SceneManager.CurrentScene;

            if (!EditorGUI.CollapsingHeader(scene.Name, defaultOpen: true))
            {
                return;
            }

            List<GameObject> gameObjects = scene.RootGameObjects;

            for (int i = 0; i < gameObjects.Count; i++)
            {
                using (new EditorGUI.IDScope(i))
                {
                    DrawGameObjectsRecursive(gameObjects[i]);
                }
            }

            s_ContextMenu.DoWindowContext();
        }

        private static void DrawGameObjectsRecursive(GameObject go)
        {
            bool isSelected = Selection.Active == go;
            bool isLeaf = go.transform.ChildCount == 0;

            if (EditorGUI.BeginTreeNode(go.Name, selected: isSelected, isLeaf: isLeaf, openOnArrow: true, openOnDoubleClick: true, spanWidth: true))
            {
                if (EditorGUI.IsItemClicked())
                {
                    Selection.Active = go;
                }

                for (int i = 0; i < go.transform.ChildCount; i++)
                {
                    using (new EditorGUI.IDScope(i))
                    {
                        DrawGameObjectsRecursive(go.transform.GetChild(i).gameObject);
                    }
                }

                EditorGUI.EndTreeNode();
            }
        }
    }
}
