using March.Core;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.IconFont;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Material Asset", ".mat", Version = 3)]
    public class MaterialImporter : DirectAssetImporter
    {
        protected override MarchObject CreateAsset()
        {
            return new Material();
        }

        protected override string? GetNormalIcon()
        {
            return FontAwesome6.Droplet;
        }
    }

    internal class MaterialImporterDrawer : DirectAssetImporterDrawerFor<MaterialImporter>
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            var material = (Material)Target.MainAsset;
            var contract = (JsonObjectContract)PersistentManager.ResolveJsonContract(material.GetType());

            bool isChanged = EditorGUI.PropertyField(contract.GetEditorProperty(material, "Shader"));

            if (material.Shader != null)
            {
                foreach (ShaderProperty shaderProp in material.Shader.Properties)
                {
                    switch (shaderProp.Type)
                    {
                        case ShaderPropertyType.Float:
                            {
                                isChanged |= DrawFloat(material, shaderProp);
                                break;
                            }

                        case ShaderPropertyType.Int:
                            {
                                int value = material.MustGetInt(shaderProp.Name);
                                isChanged |= EditorGUI.IntField(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetInt(shaderProp.Name, value);
                                break;
                            }

                        case ShaderPropertyType.Color:
                            {
                                isChanged |= DrawColor(material, shaderProp);
                                break;
                            }

                        case ShaderPropertyType.Vector:
                            {
                                Vector4 value = material.MustGetVector(shaderProp.Name);
                                isChanged |= EditorGUI.Vector4Field(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetVector(shaderProp.Name, value);
                                break;
                            }

                        case ShaderPropertyType.Texture:
                            {
                                Texture? value = material.MustGetTexture(shaderProp.Name);
                                bool isTexChanged = EditorGUI.MarchObjectField(shaderProp.Label, shaderProp.Tooltip, ref value);

                                if (isTexChanged && (value == null || value.Desc.Dimension == shaderProp.TexDimension))
                                {
                                    isChanged = true;
                                    material.SetTexture(shaderProp.Name, value);
                                }

                                break;
                            }
                    }
                }
            }

            showApplyRevertButtons = true;
            return isChanged;
        }

        private static bool DrawFloat(Material material, ShaderProperty prop)
        {
            bool isChanged;
            float value = material.MustGetFloat(prop.Name);
            ShaderPropertyAttribute? rangeAttr = prop.Attributes.FirstOrDefault(a => a.Name == "Range");

            if (rangeAttr == null || rangeAttr.Arguments == null)
            {
                isChanged = EditorGUI.FloatField(prop.Label, prop.Tooltip, ref value);
            }
            else
            {
                float[] limits = rangeAttr.Arguments.Split(',').Select(s => float.Parse(s.Trim())).ToArray();
                isChanged = EditorGUI.FloatSliderField(prop.Label, prop.Tooltip, ref value, limits[0], limits[1]);
            }

            material.SetFloat(prop.Name, value);
            return isChanged;
        }

        private static bool DrawColor(Material material, ShaderProperty prop)
        {
            Color value = material.MustGetColor(prop.Name);
            bool hdr = prop.Attributes.Exists(a => a.Name == "HDR");
            bool isChanged = EditorGUI.ColorField(prop.Label, prop.Tooltip, ref value, hdr: hdr);
            material.SetColor(prop.Name, value);
            return isChanged;
        }
    }
}
