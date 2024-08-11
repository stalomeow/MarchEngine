using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor.Windows
{
    internal static class HierarchyWindow
    {
        private static readonly PopupMenu s_ContextMenu = new("HierarchyContextMenu");

        static HierarchyWindow()
        {
            s_ContextMenu.AddMenuItem("Create/Primitive/Cube", () =>
            {
                var go = new GameObject() { Name = "Cube" };
                var mr = go.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Cube;

                SceneManager.CurrentScene.RootGameObjects.Add(go);
                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Primitive/Sphere", () =>
            {
                var go = new GameObject() { Name = "Sphere" };
                var mr = go.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Sphere;

                SceneManager.CurrentScene.RootGameObjects.Add(go);
                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Light", () =>
            {
                var go = new GameObject() { Name = "Directional Light" };
                go.AddComponent<Light>();

                SceneManager.CurrentScene.RootGameObjects.Add(go);
                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Empty", () =>
            {
                var go = new GameObject();
                SceneManager.CurrentScene.RootGameObjects.Add(go);
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
                GameObject go = gameObjects[i];

                using (new EditorGUI.IDScope(i))
                {
                    if (EditorGUI.BeginTreeNode(go.Name, selected: (Selection.Active == go), isLeaf: true, spanWidth: true))
                    {
                        if (EditorGUI.IsItemClicked())
                        {
                            Selection.Active = go;
                        }

                        EditorGUI.EndTreeNode();
                    }
                }
            }

            s_ContextMenu.DoWindowContext();
        }
    }
}
