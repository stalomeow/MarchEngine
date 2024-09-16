using March.Core;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.Importers;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace March.Editor.Drawers
{
    internal class MaterialImporterDrawer : DirectAssetImporterDrawerFor<MaterialImporter>
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            var material = (Material)Target.Asset;
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
                                int value = material.GetInt(shaderProp.Name, shaderProp.DefaultInt);
                                isChanged |= EditorGUI.IntField(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetInt(shaderProp.Name, value);
                                break;
                            }

                        case ShaderPropertyType.Color:
                            {
                                Color value = material.GetColor(shaderProp.Name, shaderProp.DefaultColor);
                                isChanged |= EditorGUI.ColorField(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetColor(shaderProp.Name, value);
                                break;
                            }

                        case ShaderPropertyType.Vector:
                            {
                                Vector4 value = material.GetVector(shaderProp.Name, shaderProp.DefaultVector);
                                isChanged |= EditorGUI.Vector4Field(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetVector(shaderProp.Name, value);
                                break;
                            }

                        case ShaderPropertyType.Texture:
                            {
                                Texture? value = material.GetTexture(shaderProp.Name);
                                isChanged |= EditorGUI.EngineObjectField(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetTexture(shaderProp.Name, value);
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
            float value = material.GetFloat(prop.Name, prop.DefaultFloat);
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
    }
}
