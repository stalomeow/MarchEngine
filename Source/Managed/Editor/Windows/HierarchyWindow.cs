using March.Core;
using March.Core.IconFont;
using March.Core.Rendering;
using March.Editor.AssetPipeline.Importers;
using System.Numerics;
using System.Text;

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
                var go = SceneManager.CurrentScene.CreateGameObject("Cube", Selection.FirstOrDefault<GameObject>());
                var renderer = go.AddComponent<MeshRenderer>();
                renderer.Mesh = Mesh.GetGeometry(MeshGeometry.Cube);
                renderer.Materials.Add(AssetManager.Load<Material>("Engine/Resources/Materials/DefaultLit.mat"));
                renderer.SyncNativeMaterials();

                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Create/Geometry/Sphere", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Sphere", Selection.FirstOrDefault<GameObject>());
                var renderer = go.AddComponent<MeshRenderer>();
                renderer.Mesh = Mesh.GetGeometry(MeshGeometry.Sphere);
                renderer.Materials.Add(AssetManager.Load<Material>("Engine/Resources/Materials/DefaultLit.mat"));
                renderer.SyncNativeMaterials();

                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Create/Light/Directional", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Directional Light", Selection.FirstOrDefault<GameObject>());
                var light = go.AddComponent<Light>();
                light.Type = LightType.Directional;

                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Create/Light/Point", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Point Light", Selection.FirstOrDefault<GameObject>());
                var light = go.AddComponent<Light>();
                light.Type = LightType.Point;

                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Create/Light/Spot", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Spot Light", Selection.FirstOrDefault<GameObject>());
                var light = go.AddComponent<Light>();
                light.Type = LightType.Spot;

                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Create/Camera", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject("Camera", Selection.FirstOrDefault<GameObject>());
                go.AddComponent<Camera>();

                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Create/Empty", (ref object? arg) =>
            {
                var go = SceneManager.CurrentScene.CreateGameObject(parent: Selection.FirstOrDefault<GameObject>());
                Selection.Set(go);
            });

            s_ContextMenu.AddMenuItem("Delete", (ref object? arg) =>
            {
                for (int i = 0; i < Selection.Count; i++)
                {
                    if (Selection.Get(i) is not GameObject go)
                    {
                        continue;
                    }

                    go.transform.Detach();
                    SceneManager.CurrentScene.RootGameObjects.Remove(go);
                    go.Dispose();
                }

                Selection.Clear();

            }, enabled: (ref object? arg) => Selection.All<GameObject>());
        }

        private readonly HierarchyTreeView m_TreeView = new();

        public HierarchyWindow() : base(FontAwesome6.BarsStaggered, "Hierarchy")
        {
            DefaultSize = new Vector2(350.0f, 600.0f);
        }

        protected override void OnDraw()
        {
            base.OnDraw();

            //bool enableDragDrop = false;
            Scene scene = SceneManager.CurrentScene;
            using var sceneLabel = EditorGUIUtility.BuildIconText(SceneImporter.SceneIcon, scene.Name);

            if (EditorGUI.CollapsingHeader(sceneLabel, defaultOpen: true))
            {
                m_TreeView.Draw();
                s_ContextMenu.ShowAsWindowContext();
                //enableDragDrop = true;
            }
            else if (EditorGUI.IsWindowClicked())
            {
                // 点在空白上取消选择，这样用户体验更好一点
                Selection.Clear();
            }

            //HandleDragDrop(scene, enableDragDrop);
        }

        private static void HandleDragDrop(Scene scene, bool enabled)
        {
            if (!DragDrop.BeginTarget(DragDropArea.Window, out DragDropData data))
            {
                return;
            }

            if (enabled && data.Payload is GameObject go)
            {
                if (data.IsDelivery)
                {
                    scene.AddRootGameObject(Instantiate(go));
                }

                DragDrop.EndTarget(DragDropResult.AcceptByRect);
            }
            else
            {
                DragDrop.EndTarget(DragDropResult.Reject);
            }
        }
    }

    internal class HierarchyTreeView : TreeView
    {
        protected override int GetChildCount(object? item)
        {
            if (item == null)
            {
                return SceneManager.CurrentScene.RootGameObjects.Count;
            }

            return ((GameObject)item).transform.ChildCount;
        }

        protected override object GetChildItem(object? item, int index)
        {
            if (item == null)
            {
                return SceneManager.CurrentScene.RootGameObjects[index];
            }

            return ((GameObject)item).transform.GetChild(index).gameObject;
        }

        protected override TreeViewItemDesc GetItemDesc(object item, StringBuilder labelBuilder)
        {
            GameObject go = (GameObject)item;
            labelBuilder.AppendIconText(FontAwesome6.DiceD6, go.Name, "GameObject");

            return new TreeViewItemDesc
            {
                IsDisabled = !go.IsActiveSelf, // 因为是从上往下递归绘制的，所以只要判断当前节点是否激活即可
                SelectionObject = go,
            };
        }
    }
}
