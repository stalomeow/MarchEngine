using DX12Demo.Core;
using Newtonsoft.Json.Serialization;
using System.Numerics;
using System.Runtime.InteropServices;

namespace DX12Demo.Editor
{
    internal static unsafe class Inspector
    {
        private static int s_SelectedGameObjectIndex = -1;

        [UnmanagedCallersOnly]
        internal static void Draw()
        {
            // No selection
            //if (s_SelectedGameObjectIndex < 0)
            //{
            //    return;
            //}

            s_SelectedGameObjectIndex = 0;

            List<GameObject> gameObjects = SceneManager.CurrentScene.RootGameObjects;

            if (s_SelectedGameObjectIndex >= gameObjects.Count)
            {
                s_SelectedGameObjectIndex = -1;
                return;
            }

            GameObject go = gameObjects[s_SelectedGameObjectIndex];
            DrawProperties(go, ((JsonObjectContract)PersistentManager.ResolveJsonContract(go.GetType())).Properties);

            EditorGUI.SeparatorText("Components");

            for (int i = 0; i < go.m_Components.Count; i++)
            {
                Component component = go.m_Components[i];

                if (PersistentManager.ResolveJsonContract(component.GetType()) is not JsonObjectContract contract)
                {
                    Debug.Assert(false, "Failed to resolve json contract.");
                    continue;
                }

                if (!EditorGUI.CollapsingHeader(contract.UnderlyingType.Name, defaultOpen: true))
                {
                    continue;
                }

                DrawProperties(component, contract.Properties);
            }

            EditorGUI.Space();

            if (EditorGUI.CenterButton("Add Component", 220.0f))
            {
                go.AddComponent<DX12Demo.Core.Rendering.MeshRenderer>();
            }
        }

        private static void DrawProperties(object target, JsonPropertyCollection properties)
        {
            foreach (var prop in properties)
            {
                if (prop.Ignored)
                {
                    continue;
                }

                if (s_DefaultPropertyDrawers.TryGetValue(prop.PropertyType!, out Action<object, JsonProperty>? drawer))
                {
                    drawer(target, prop);
                    continue;
                }

                if (prop.PropertyType is { IsEnum: true })
                {
                    object value = prop.ValueProvider?.GetValue(target)!;
                    if (EditorGUI.EnumField(prop.PropertyName!, ref value))
                    {
                        prop.ValueProvider!.SetValue(target, value);
                    }
                    continue;
                }

                // fallback
                using (new EditorGUI.DisabledScope())
                {
                    EditorGUI.LabelField(prop.PropertyName!, $"Type {prop.PropertyType?.Name} is not supported");
                }
            }
        }

        private static readonly Dictionary<Type, Action<object, JsonProperty>> s_DefaultPropertyDrawers = new()
        {
            [typeof(float)] = (target, prop) =>
            {
                float value = (float)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.FloatField(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(Vector2)] = (target, prop) =>
            {
                Vector2 value = (Vector2)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.Vector2Field(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(Vector3)] = (target, prop) =>
            {
                Vector3 value = (Vector3)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.Vector3Field(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(Vector4)] = (target, prop) =>
            {
                Vector4 value = (Vector4)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.Vector4Field(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(Color)] = (target, prop) =>
            {
                Color value = (Color)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.ColorField(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(Rotator)] = (target, prop) =>
            {
                Rotator value = (Rotator)prop.ValueProvider?.GetValue(target)!;
                Vector3 eulerAngles = value.EulerAngles;
                if (EditorGUI.Vector3Field(prop.PropertyName!, ref eulerAngles))
                {
                    value.EulerAngles = eulerAngles;
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(string)] = (target, prop) =>
            {
                string value = (string)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.TextField(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
            [typeof(bool)] = (target, prop) =>
            {
                bool value = (bool)prop.ValueProvider?.GetValue(target)!;
                if (EditorGUI.Checkbox(prop.PropertyName!, ref value))
                {
                    prop.ValueProvider!.SetValue(target, value);
                }
            },
        };
    }
}
