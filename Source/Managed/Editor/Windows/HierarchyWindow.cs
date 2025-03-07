using March.Core;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
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
            s_ContextMenu.AddMenuItem("Create/Geometry/Cube", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Cube", Selection.Active as GameObject);
                var renderer = go.AddComponent<MeshRenderer>();
                renderer.Mesh = Mesh.GetGeometry(MeshGeometry.Cube);

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Geometry/Sphere", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Sphere", Selection.Active as GameObject);
                var renderer = go.AddComponent<MeshRenderer>();
                renderer.Mesh = Mesh.GetGeometry(MeshGeometry.Sphere);

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Light/Directional", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Directional Light", Selection.Active as GameObject);
                var light = go.AddComponent<Light>();
                light.Type = LightType.Directional;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Light/Point", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Point Light", Selection.Active as GameObject);
                var light = go.AddComponent<Light>();
                light.Type = LightType.Point;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Light/Spot", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Spot Light", Selection.Active as GameObject);
                var light = go.AddComponent<Light>();
                light.Type = LightType.Spot;

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Camera", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Camera", Selection.Active as GameObject);
                go.AddComponent<Camera>();

                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Create/Empty", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject(parent: Selection.Active as GameObject);
                Selection.Active = go;
            });

            s_ContextMenu.AddMenuItem("Delete", (ref object? arg) =>
            {
                if (Selection.Active is not GameObject go)
                {
                    return;
                }

                go.transform.Detach();
                SceneManager.CurrentScene.RootGameObjects.Remove(go);
                go.Dispose();

                Selection.Active = null;

            }, enabled: (ref object? arg) => Selection.Active is GameObject);
        }

        public HierarchyWindow() : base(FontAwesome6.BarsStaggered, "Hierarchy")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            bool enableDragDrop = false;
            Scene scene = SceneManager.CurrentScene;
            using var sceneLabel = EditorGUIUtility.BuildIconText(SceneImporter.SceneIcon, scene.Name);
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
                enableDragDrop = true;
            }
            else if (EditorGUI.IsWindowClicked())
            {
                // 点在空白上取消选择，这样用户体验更好一点
                Selection.Active = null;
            }

            HandleDragDrop(scene, enableDragDrop);
        }

        private static void HandleDragDrop(Scene scene, bool enabled)
        {
            if (!DragDrop.BeginTarget(DragDropArea.Window, out MarchObject? payload, out bool isDelivery))
            {
                return;
            }

            if (enabled && payload is GameObject go)
            {
                if (isDelivery)
                {
                    scene.AddRootGameObject(Instantiate(go));
                }

                DragDrop.EndTarget(DragDropResult.Accept);
            }
            else
            {
                DragDrop.EndTarget(DragDropResult.Reject);
            }
        }

        private static void DrawGameObjectsRecursive(GameObject go, List<GameObject> selections, ref bool isAnyItemClicked)
        {
            // 因为是从上往下递归绘制的，所以只要判断当前节点是否激活即可
            using (new EditorGUI.DisabledScope(!go.IsActiveSelf, allowInteraction: true))
            {
                using var label = EditorGUIUtility.BuildIconText(FontAwesome6.DiceD6, go.Name, "GameObject");
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
}
