using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.AssetPipeline.Importers;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Hierarchy")]
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

            s_ContextMenu.AddMenuItem("Save", (ref object? arg) =>
            {
                Scene s = SceneManager.CurrentScene;
                string json = PersistentManager.SaveAsString(s);
                Debug.LogInfo(json);
            });

            s_ContextMenu.AddMenuItem("Load Sponza", (ref object? arg) =>
            {
                GameObject? go = AssetDatabase.Load<GameObject>("Assets/Models/Sponza/Sponza.gltf");

                if (go != null)
                {
                    SceneManager.CurrentScene.AddRootGameObject(Instantiate(go));
                }
            });

            s_ContextMenu.AddMenuItem("Load Fox", (ref object? arg) =>
            {
                GameObject? go = AssetDatabase.Load<GameObject>("Assets/Models/Fox/Fox.gltf");

                if (go != null)
                {
                    SceneManager.CurrentScene.AddRootGameObject(Instantiate(go));
                }
            });

            s_ContextMenu.AddMenuItem("Create Material", (ref object? arg) =>
            {
                string path = Application.SaveFilePanelInProject("Save Material", "New Material", "mat");

                if (!string.IsNullOrEmpty(path))
                {
                    AssetDatabase.Create(path, new Material());
                }
            });
        }

        public HierarchyWindow() : base($"{FontAwesome6.BarsStaggered} Hierarchy")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            Scene scene = SceneManager.CurrentScene;
            string sceneLabel = $"{SceneImporter.SceneIcon} {scene.Name}###{scene.Name}";
            using var selections = ListPool<GameObject>.Get();

            if (EditorGUI.CollapsingHeader(sceneLabel, defaultOpen: true))
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
            string label = $"{FontAwesome6.DiceD6} {go.Name}###GameObject";
            bool isSelected = Selection.Active == go;
            bool isLeaf = go.transform.ChildCount == 0;
            bool isOpen = EditorGUI.BeginTreeNode(label, selected: isSelected, isLeaf: isLeaf, openOnArrow: true, openOnDoubleClick: true, spanWidth: true);

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
