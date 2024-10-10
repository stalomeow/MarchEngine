using March.Core;
using March.Core.Rendering;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Hierarchy")]
    internal class HierarchyWindow : EditorWindow
    {
        private static readonly GenericMenu s_ContextMenu = new("HierarchyContextMenu");

        static HierarchyWindow()
        {
            s_ContextMenu.AddMenuItem("Create/Primitive/Cube", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Cube", Selection.Active as GameObject);
                var mr = go.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Cube;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Primitive/Sphere", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Sphere", Selection.Active as GameObject);
                var mr = go.AddComponent<MeshRenderer>();
                mr.MeshType = MeshType.Sphere;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Light", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Directional Light", Selection.Active as GameObject);
                go.AddComponent<Light>();

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Empty", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject(parent: Selection.Active as GameObject);
                Selection.Active = go;
            });
        }

        public HierarchyWindow() : base("Hierarchy") { }

        protected override void OnDraw()
        {
            base.OnDraw();

            Scene scene = SceneManager.CurrentScene;
            using var selections = ListPool<GameObject>.Get();

            if (EditorGUI.CollapsingHeader(scene.Name, defaultOpen: true))
            {
                List<GameObject> gameObjects = scene.RootGameObjects;
                bool isAnyItemClicked = false;

                for (int i = 0; i < gameObjects.Count; i++)
                {
                    using (new EditorGUI.IDScope(i))
                    {
                        DrawGameObjectsRecursive(gameObjects[i], selections, ref isAnyItemClicked);
                    }
                }

                // 在 window context menu 之前处理选择
                if (selections.Value.Count > 0)
                {
                    Selection.Active = selections.Value[0];
                }
                else if (!isAnyItemClicked && EditorGUI.IsWindowClicked())
                {
                    // 点在空白上取消选择，这样用户体验更好一点
                    Selection.Active = null;
                }

                s_ContextMenu.ShowAsWindowContext();
            }
            else if (EditorGUI.IsWindowClicked())
            {
                // 点在空白上取消选择，这样用户体验更好一点
                Selection.Active = null;
            }
        }

        private static void DrawGameObjectsRecursive(GameObject go, List<GameObject> selections, ref bool isAnyItemClicked)
        {
            bool isSelected = Selection.Active == go;
            bool isLeaf = go.transform.ChildCount == 0;
            bool isOpen = EditorGUI.BeginTreeNode(go.Name, selected: isSelected, isLeaf: isLeaf, openOnArrow: true, openOnDoubleClick: true, spanWidth: true);

            EditorGUI.ItemClickResult clickResult = EditorGUI.IsTreeNodeClicked(isOpen, isLeaf);
            isAnyItemClicked |= clickResult != EditorGUI.ItemClickResult.False;

            // 点箭头不修改 selection
            if (clickResult == EditorGUI.ItemClickResult.True)
            {
                selections.Add(go);
            }

            if (isOpen)
            {
                for (int i = 0; i < go.transform.ChildCount; i++)
                {
                    using (new EditorGUI.IDScope(i))
                    {
                        DrawGameObjectsRecursive(go.transform.GetChild(i).gameObject, selections, ref isAnyItemClicked);
                    }
                }

                EditorGUI.EndTreeNode();
            }
        }
    }
}
