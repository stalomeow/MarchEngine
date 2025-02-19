using March.Core;
using March.Core.Diagnostics;
using March.Core.Interop;
using March.Core.Pool;
using March.Core.Serialization;
using March.Editor.AssetPipeline;
using March.Editor.Windows;
using Newtonsoft.Json;
using System.Reflection;
using System.Runtime.InteropServices;

#pragma warning disable IDE0051 // Remove unused private members

namespace March.Editor
{
    public partial class EditorApplication : Application
    {
        private sealed class WindowData : MarchObject
        {
            [JsonProperty]
            public List<EditorWindow> Windows { get; set; } = [];

            [JsonProperty]
            public Dictionary<Type, Stack<string>> IdPool { get; set; } = [];
        }

        private static readonly WindowData s_WindowData = new();
        private static readonly GenericMenu s_WindowMenu = new("EditorWindowMenu");
        private static int? s_LastTypeCacheVersion;

        [UnmanagedCallersOnly]
        private static void Initialize()
        {
            AssetDatabase.Initialize();
            SceneManager.InitializeEditor(selected: go => Selection.Active == go);

            LoadWindows();

            OnTick += () =>
            {
                ShowWindowMenu();
                DrawWindows();
            };

            OnQuit += () =>
            {
                SaveWindows();
                DisposeWindows();
            };
        }

        [UnmanagedCallersOnly]
        private static void OpenConsoleWindowIfNot()
        {
            OpenWindowIfNot<ConsoleWindow>();
        }

        public static T OpenWindow<T>() where T : EditorWindow, new()
        {
            var window = new T();
            OnWindowOpened(window);
            return window;
        }

        public static T? OpenWindowIfNot<T>() where T : EditorWindow, new()
        {
            if (s_WindowData.Windows.Find(w => w is T) != null)
            {
                return null;
            }

            return OpenWindow<T>();
        }

        public static EditorWindow? OpenWindow(Type windowType)
        {
            try
            {
                var window = (EditorWindow)Activator.CreateInstance(windowType, true)!;
                OnWindowOpened(window);
                return window;
            }
            catch (Exception e)
            {
                Log.Message(LogLevel.Error, "Failed to open window", $"{windowType} {e}");
                return null;
            }
        }

        public static EditorWindow? OpenWindowIfNot(Type windowType)
        {
            if (s_WindowData.Windows.Find(w => w.GetType() == windowType) != null)
            {
                return null;
            }

            return OpenWindow(windowType);
        }

        private static void OnWindowOpened(EditorWindow window)
        {
            Type windowType = window.GetType();

            // 循环使用 id
            if (s_WindowData.IdPool.TryGetValue(windowType, out Stack<string>? pool))
            {
                window.Id = pool.Pop();

                if (pool.Count <= 0)
                {
                    s_WindowData.IdPool.Remove(windowType);
                }
            }

            s_WindowData.Windows.Add(window);
        }

        private static void DrawWindows()
        {
            // 正向遍历，这样 DrawWindows() 中可以 OpenWindow()，且不会影响遍历
            for (int i = 0; i < s_WindowData.Windows.Count; i++)
            {
                EditorWindow window = s_WindowData.Windows[i];
                window.Draw();

                if (!window.IsOpen)
                {
                    OnWindowClosed(window);
                    s_WindowData.Windows.RemoveAt(i);
                    i--;
                }
            }
        }

        private static void OnWindowClosed(EditorWindow window)
        {
            Type windowType = window.GetType();

            if (!s_WindowData.IdPool.TryGetValue(windowType, out Stack<string>? pool))
            {
                pool = new Stack<string>();
                s_WindowData.IdPool.Add(windowType, pool);
            }

            // 关闭窗口后，ImGui 不会删除窗口相关设置
            // 如果不循环使用 id，那么 ini 配置文件会越来越大，占用内存也越来越多
            // ImGui 底层用于保存窗口设置的 ImChunkStream<T> 只能往后添加元素，不能删除元素
            pool.Push(window.Id);
            window.Dispose();
        }

        private static void ShowWindowMenu()
        {
            RebuildWindowMenuIfNeeded();
            s_WindowMenu.ShowInMainMenuBar();
        }

        private static void RebuildWindowMenuIfNeeded()
        {
            if (s_LastTypeCacheVersion == TypeCache.Version)
            {
                return;
            }

            s_LastTypeCacheVersion = TypeCache.Version;
            s_WindowMenu.Clear();

            using var types = ListPool<KeyValuePair<string, Type>>.Get();

            foreach (Type windowType in TypeCache.GetTypesDerivedFrom<EditorWindow>())
            {
                var attr = windowType.GetCustomAttribute<EditorWindowMenuAttribute>();

                if (attr == null)
                {
                    continue;
                }

                types.Value.Add(new KeyValuePair<string, Type>(attr.MenuPath, windowType));
            }

            types.Value.Sort((x, y) =>
            {
                int slashCountX = x.Key.Count(c => c == '/');
                int slashCountY = y.Key.Count(c => c == '/');

                // 斜杠越多的越靠前
                if (slashCountX != slashCountY)
                {
                    return slashCountY.CompareTo(slashCountX);
                }

                return x.Key.CompareTo(y.Key);
            });

            foreach (KeyValuePair<string, Type> kv in types.Value)
            {
                Type windowType = kv.Value;
                s_WindowMenu.AddMenuItem(kv.Key, callback: (ref object? _) => OpenWindow(windowType));
            }
        }

        private static string GetWindowDataFilePath()
        {
            return Path.Combine(Application.DataPath, "ProjectSettings", "EditorWindows.json");
        }

        private static void LoadWindows()
        {
            string path = GetWindowDataFilePath();

            if (File.Exists(path))
            {
                PersistentManager.Overwrite(path, s_WindowData);
            }
            else
            {
                // 默认窗口
                var scene = new SceneWindow();
                var inspector = new InspectorWindow();
                var hierarchy = new HierarchyWindow();
                var project = new ProjectWindow();

                EditorWindow.SplitDockNode(EditorWindow.MainViewportDockSpaceNode,
                    DockNodeSplitDir.Left, 0.65f, out uint sceneNode, out uint rightNode);
                EditorWindow.SplitDockNode(rightNode,
                    DockNodeSplitDir.Left, 0.5f, out uint leftNode, out uint inspectorNode);
                EditorWindow.SplitDockNode(leftNode,
                    DockNodeSplitDir.Up, 0.5f, out uint hierarchyNode, out uint projectNode);

                scene.DockIntoNode(sceneNode);
                inspector.DockIntoNode(inspectorNode);
                hierarchy.DockIntoNode(hierarchyNode);
                project.DockIntoNode(projectNode);

                EditorWindow.ApplyModificationsInChildDockNodes(EditorWindow.MainViewportDockSpaceNode);

                s_WindowData.Windows.Add(scene);
                s_WindowData.Windows.Add(inspector);
                s_WindowData.Windows.Add(hierarchy);
                s_WindowData.Windows.Add(project);
            }
        }

        private static void SaveWindows()
        {
            PersistentManager.Save(s_WindowData, GetWindowDataFilePath());
        }

        private static void DisposeWindows()
        {
            foreach (EditorWindow window in s_WindowData.Windows)
            {
                window.Dispose();
            }
        }

        [NativeMethod]
        public static partial string SaveFilePanelInProject(string title, string defaultName, string extension, string path = "Assets");
    }
}
