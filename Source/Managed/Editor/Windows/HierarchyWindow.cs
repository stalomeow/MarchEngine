using March.Core;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Editor.AssetPipeline.Importers;
using System.Numerics;
using System.Runtime.InteropServices;
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
                using var gameObjects = ListPool<GameObject>.Get();
                Selection.GetTopLevelGameObjects(gameObjects);

                foreach (GameObject go in gameObjects.Value)
                {
                    go.transform.Detach();
                    SceneManager.CurrentScene.RootGameObjects.Remove(go);
                    go.Dispose();
                }

                Selection.Objects.Clear();

            }, enabled: (ref object? arg) => Selection.All<GameObject>());
        }

        private readonly HierarchyTreeView m_TreeView = new("GameObjectHierarchy");

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

            if (EditorGUI.CollapsingHeader(sceneLabel, defaultOpen: true))
            {
                m_TreeView.Draw();
                s_ContextMenu.ShowAsWindowContext();
                enableDragDrop = true;
            }

            if (EditorGUI.IsNothingClickedOnWindow())
            {
                Selection.Objects.Clear();
            }

            HandleDragDrop(scene, enableDragDrop);
        }

        private static void HandleDragDrop(Scene scene, bool enabled)
        {
            if (!DragDrop.BeginTarget(DragDropArea.Window))
            {
                return;
            }

            if (enabled && DragDrop.Objects.Count > 0 && DragDrop.Objects.All(obj => obj is IPrefabProvider))
            {
                if (DragDrop.IsDelivery)
                {
                    Selection.Objects.Clear();

                    foreach (MarchObject obj in DragDrop.Objects)
                    {
                        GameObject prefab = ((IPrefabProvider)obj).GetPrefab();
                        GameObject go = Instantiate(prefab);
                        SceneManager.CurrentScene.AddRootGameObject(go);
                        Selection.Objects.Add(go);
                    }
                }

                DragDrop.EndTarget(DragDropResult.AcceptByRect);
            }
            else
            {
                DragDrop.EndTarget(DragDropResult.Reject);
            }
        }
    }

    internal class HierarchyTreeView(string treeViewUniqueName) : TreeView(treeViewUniqueName)
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

        protected override object? GetItemByEngineObject(MarchObject obj)
        {
            return obj is GameObject ? obj : null;
        }

        protected override TreeViewItemDesc GetItemDesc(object item, StringBuilder labelBuilder)
        {
            GameObject go = (GameObject)item;
            labelBuilder.AppendIconText(FontAwesome6.DiceD6, go.Name, "GameObject");

            return new TreeViewItemDesc
            {
                IsDisabled = !go.IsActiveSelf, // 因为是从上往下递归绘制的，所以只要判断当前节点是否激活即可
                EngineObject = go,
            };
        }

        protected override void GetDragTooltip(IReadOnlyList<object> items, StringBuilder tooltipBuilder)
        {
            using var paths = ListPool<string?>.Get();

            for (int i = 0; i < items.Count; i++)
            {
                GameObject go = (GameObject)items[i];

                // 获取物体的路径
                paths.Value.Clear();
                Transform? transform = go.transform;
                while (transform != null)
                {
                    paths.Value.Add(transform.gameObject.Name);
                    transform = transform.Parent;
                }
                paths.Value.Reverse();

                tooltipBuilder.Append(FontAwesome6.DiceD6).Append(' ');
                tooltipBuilder.AppendJoin('/', CollectionsMarshal.AsSpan(paths.Value));
                tooltipBuilder.AppendLine();
            }
        }

        protected override bool CanMoveItem(in TreeViewItemMoveData data)
        {
            return true;
        }

        protected override void OnMoveItem(in TreeViewItemMoveData data)
        {
            Transform moving = ((GameObject)data.MovingItem).transform;
            Transform target = ((GameObject)data.TargetItem).transform;
            HandleDropTransform(moving, target, data.Position);
        }

        protected override bool CanHandleExternalDrop(in TreeViewExternalDropData data)
        {
            return data.Objects.Count > 0 && data.Objects.All(obj => obj is IPrefabProvider);
        }

        protected override void OnHandleExternalDrop(in TreeViewExternalDropData data)
        {
            Transform target = ((GameObject)data.TargetItem).transform;
            Selection.Objects.Clear();

            foreach (MarchObject obj in data.Objects)
            {
                GameObject prefab = ((IPrefabProvider)obj).GetPrefab();
                GameObject go = MarchObject.Instantiate(prefab);
                SceneManager.CurrentScene.AddRootGameObject(go);
                HandleDropTransform(go.transform, target, data.Position);
                Selection.Objects.Add(go);
            }
        }

        private static void HandleDropTransform(Transform transform, Transform target, TreeViewDropPosition position)
        {
            switch (position)
            {
                case TreeViewDropPosition.AboveItem:
                    transform.SetParent(target.Parent);
                    transform.SetSiblingIndex(target.GetSiblingIndex());
                    break;
                case TreeViewDropPosition.OnItem:
                    transform.SetParent(target);
                    break;
                case TreeViewDropPosition.BelowItem:
                    transform.SetParent(target.Parent);
                    transform.SetSiblingIndex(target.GetSiblingIndex() + 1);
                    break;
                default:
                    throw new NotSupportedException("Unexpected drop position: " + position);
            }
        }
    }
}
