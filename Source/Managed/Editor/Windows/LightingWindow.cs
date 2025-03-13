using March.Core;
using March.Core.IconFont;
using March.Core.Rendering;
using March.Editor.AssetPipeline;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Lighting")]
    internal class LightingWindow : EditorWindow
    {
        public LightingWindow() : base(FontAwesome6.Lightbulb, "Lighting") { }

        protected override void OnDraw()
        {
            base.OnDraw();

            if (EditorGUI.CollapsingHeader("Environment", defaultOpen: true))
            {
                Scene scene = EditorSceneManager.CurrentScene;

                Material? skybox = scene.SkyboxMaterial;
                if (EditorGUI.MarchObjectField("Skybox", "", ref skybox))
                {
                    scene.SkyboxMaterial = skybox;
                    AssetDatabase.SetDirty(scene);

                    RenderPipeline.SetSkyboxMaterial(skybox);
                }

                Texture? radianceMap = scene.EnvironmentRadianceMap;
                if (EditorGUI.MarchObjectField("Radiance Map", "", ref radianceMap))
                {
                    if (radianceMap == null || radianceMap.Desc.Dimension == TextureDimension.Cube)
                    {
                        scene.EnvironmentRadianceMap = radianceMap;
                        AssetDatabase.SetDirty(scene);
                    }
                }

                float diffuseIntensityMultiplier = scene.EnvironmentDiffuseIntensityMultiplier;
                if (EditorGUI.FloatSliderField("Diffuse Intensity Multiplier", "", ref diffuseIntensityMultiplier, 0.0f, 1.0f))
                {
                    scene.EnvironmentDiffuseIntensityMultiplier = diffuseIntensityMultiplier;
                    AssetDatabase.SetDirty(scene);
                }

                float specularIntensityMultiplier = scene.EnvironmentSpecularIntensityMultiplier;
                if (EditorGUI.FloatSliderField("Specular Intensity Multiplier", "", ref specularIntensityMultiplier, 0.0f, 1.0f))
                {
                    scene.EnvironmentSpecularIntensityMultiplier = specularIntensityMultiplier;
                    AssetDatabase.SetDirty(scene);
                }

                using (new EditorGUI.DisabledScope(radianceMap == null))
                {
                    if (EditorGUI.ButtonRight("Bake"))
                    {
                        RenderPipeline.BakeEnvLight(radianceMap!, diffuseIntensityMultiplier, specularIntensityMultiplier);
                    }
                }
            }
        }
    }
}
