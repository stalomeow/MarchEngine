using March.Core;
using March.Core.Diagnostics;
using March.Core.Pool;
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
            DrawGizmos((drawer, component, isSelected) => drawer.Draw(component, isSelected));
        }

        private static void OnQuit()
        {
            SaveSettings();
            AssetDatabase.SaveAssetImmediately(CurrentScene);
            DeactivateCurrentScene();
        }

        private static readonly DrawerCache<IGizmoDrawer> s_GizmoDrawerCache = new(typeof(IGizmoDrawerFor<>));

        public static void DrawGizmosGUI()
        {
            DrawGizmos((drawer, component, isSelected) => drawer.DrawGUI(component, isSelected));
        }

        private static void DrawGizmos(Action<IGizmoDrawer, Component, bool> action)
        {
            using var selections = HashSetPool<MarchObject>.Get();
            selections.Value.UnionWith(Selection.Objects);

            foreach (GameObject go in CurrentScene.RootGameObjects)
            {
                DrawGizmosRecursive(go, selections.Value, action);
            }
        }

        private static void DrawGizmosRecursive(GameObject go, HashSet<MarchObject> selections, Action<IGizmoDrawer, Component, bool> action)
        {
            if (!go.IsActiveSelf)
            {
                return;
            }

            bool isSelected = selections.Contains(go);

            DrawComponentGizmos(go.transform, isSelected, action);

            foreach (var component in go.m_Components)
            {
                DrawComponentGizmos(component, isSelected, action);
            }

            for (int i = 0; i < go.transform.ChildCount; i++)
            {
                DrawGizmosRecursive(go.transform.GetChild(i).gameObject, selections, action);
            }

            static void DrawComponentGizmos(Component component, bool isSelected, Action<IGizmoDrawer, Component, bool> action)
            {
                if (!component.IsActiveAndEnabled)
                {
                    return;
                }

                if (!s_GizmoDrawerCache.TryGetSharedInstance(component.GetType(), out IGizmoDrawer? drawer))
                {
                    return;
                }

                action(drawer, component, isSelected);
            }
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

            RenderPipeline.SetSkyboxMaterial(CurrentScene.SkyboxMaterial);

            if (CurrentScene.EnvironmentRadianceMap != null)
            {
                RenderPipeline.BakeEnvLight(
                    CurrentScene.EnvironmentRadianceMap,
                    CurrentScene.EnvironmentDiffuseIntensityMultiplier,
                    CurrentScene.EnvironmentSpecularIntensityMultiplier);
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
                    Material skybox = AssetDatabase.Load<Material>("Engine/Resources/Materials/DefaultSkybox.mat")!;
                    CurrentScene.SkyboxMaterial = skybox;
                    CurrentScene.EnvironmentRadianceMap = skybox.MustGetTexture("_Cubemap");
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
