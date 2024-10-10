using March.Core;
using March.Core.Serialization;
using March.Editor.Windows;
using Newtonsoft.Json;
using System.Reflection;
using System.Runtime.InteropServices;

namespace March.Editor
{
    public static class EditorApplication
    {
        private sealed class WindowData : MarchObject, IForceInlineSerialization
        {
            [JsonProperty]
            public List<EditorWindow> Windows { get; set; } = [];
        }

        private static readonly WindowData s_WindowData = new();
        private static readonly GenericMenu s_WindowMenu = new("EditorWindowMenu");
        private static int? s_LastTypeCacheVersion;

        [UnmanagedCallersOnly]
        private static void OnStart()
        {
            AssetDatabase.Initialize();
            LoadWindows();
        }

        [UnmanagedCallersOnly]
        private static void OnTick()
        {
            AssetDatabase.Update();
            ShowWindowMenu();
            DrawWindows();
        }

        [UnmanagedCallersOnly]
        private static void OnQuit()
        {
            SaveWindows();
        }

        private static void DrawWindows()
        {
            for (int i = s_WindowData.Windows.Count - 1; i >= 0; i--)
            {
                EditorWindow window = s_WindowData.Windows[i];
                window.Draw();

                if (!window.IsOpen)
                {
                    window.Dispose();
                    s_WindowData.Windows.RemoveAt(i);
                }
            }
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

            foreach (Type windowType in TypeCache.GetTypesDerivedFrom<EditorWindow>())
            {
                var attr = windowType.GetCustomAttribute<EditorWindowMenuAttribute>();

                if (attr == null)
                {
                    continue;
                }

                s_WindowMenu.AddMenuItem(attr.MenuPath, callback: (ref object? _) =>
                {
                    try
                    {
                        var window = (EditorWindow)Activator.CreateInstance(windowType, true)!;
                        s_WindowData.Windows.Add(window);
                    }
                    catch (Exception e)
                    {
                        Debug.LogError($"Failed to create window: {windowType}; {e}");
                    }
                });
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
                s_WindowData.Windows.Add(new SceneWindow());
                s_WindowData.Windows.Add(new HierarchyWindow());
                s_WindowData.Windows.Add(new InspectorWindow());
                s_WindowData.Windows.Add(new ProjectWindow());
                s_WindowData.Windows.Add(new ConsoleWindow());

                // TODO imgui demo window
            }
        }

        private static void SaveWindows()
        {
            PersistentManager.Save(s_WindowData, GetWindowDataFilePath());
        }
    }
}
