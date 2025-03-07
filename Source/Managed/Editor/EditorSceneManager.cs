using March.Core;
using March.Core.Diagnostics;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.AssetPipeline;
using Newtonsoft.Json;

namespace March.Editor
{
    public static class EditorSceneManager
    {
        public static Scene CurrentScene { get; private set; } = new();

        internal static void Initialize()
        {
            LoadSettingsAndInitialScene();

            SceneManager.Impl = new EditorSceneManagerImpl();

            Application.OnTick += OnUpdate;
            Application.OnQuit += OnQuit;
        }

        private static void OnUpdate()
        {
            CurrentScene.Update();

            Gizmos.Clear();
            CurrentScene.DrawGizmos(selected: go => Selection.Active == go);
        }

        private static void OnQuit()
        {
            SaveSettings();
            AssetDatabase.SaveAssetImmediately(CurrentScene);
            DeactivateCurrentScene();
        }

        public static void LoadScene(string path)
        {
            Scene? scene = AssetDatabase.Load<Scene>(path);

            if (scene == null)
            {
                Log.Message(LogLevel.Error, "Failed to load scene", $"{path}");
                return;
            }

            DeactivateCurrentScene();
            CurrentScene = scene;
            ActivateCurrentScene();
        }

        private static void ActivateCurrentScene()
        {
            foreach (var gameObject in CurrentScene.RootGameObjects)
            {
                gameObject.AwakeRecursive();
            }
        }

        private static void DeactivateCurrentScene()
        {
            CurrentScene.Dispose();
        }

        private sealed class SettingsData : MarchObject
        {
            [JsonProperty]
            public string? ActiveSceneGuid { get; set; }
        }

        private static string GetSettingsFilePath()
        {
            return Path.Combine(Application.DataPath, "ProjectSettings", "EditorSceneManager.json");
        }

        private static void LoadSettingsAndInitialScene()
        {
            bool success = false;
            string path = GetSettingsFilePath();

            if (File.Exists(path))
            {
                SettingsData data = PersistentManager.Load<SettingsData>(path);

                if (data.ActiveSceneGuid != null)
                {
                    Scene? scene = AssetDatabase.LoadByGuid<Scene>(data.ActiveSceneGuid);

                    if (scene != null)
                    {
                        CurrentScene = scene;
                        success = true;
                    }
                }
            }

            if (!success)
            {
                const string DefaultScenePath = "Assets/Scenes/NewScene.scene";
                Scene? scene = AssetDatabase.Load<Scene>(DefaultScenePath);

                if (scene != null)
                {
                    CurrentScene = scene;
                }
                else
                {
                    AssetDatabase.Create(DefaultScenePath, CurrentScene);
                }
            }

            ActivateCurrentScene();
        }

        private static void SaveSettings()
        {
            SettingsData data = new()
            {
                ActiveSceneGuid = CurrentScene.PersistentGuid
            };

            PersistentManager.Save(data, GetSettingsFilePath());
        }
    }

    file class EditorSceneManagerImpl : ISceneManagerImpl
    {
        public Scene CurrentScene => EditorSceneManager.CurrentScene;

        public void LoadScene(string path) => EditorSceneManager.LoadScene(path);
    }
}
