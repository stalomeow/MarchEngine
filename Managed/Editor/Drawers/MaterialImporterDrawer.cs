using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using DX12Demo.Core.Serialization;
using DX12Demo.Editor.Importers;
using Newtonsoft.Json.Serialization;
using System.Numerics;

namespace DX12Demo.Editor.Drawers
{
    internal class MaterialImporterDrawer : DirectAssetImporterDrawerFor<MaterialImporter>
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            var material = (Material)Target.Asset;
            var contract = (JsonObjectContract)PersistentManager.ResolveJsonContract(material.GetType());
            var isChanged = false;

            EditorProperty shaderPath = contract.GetEditorProperty(material, "m_ShaderPath");
            isChanged |= EditorGUI.PropertyField(in shaderPath);

            string shaderAssetPath = shaderPath.GetValue<string>();
            Shader? shader = AssetManager.Load<Shader>(shaderAssetPath);

            if (shader != null)
            {
                if (material.Shader != shader)
                {
                    material.SetShader(shaderAssetPath, shader);
                }

                foreach (ShaderProperty shaderProp in shader.Properties)
                {
                    switch (shaderProp.Type)
                    {
                        case ShaderPropertyType.Float:
                            {
                                float value = material.GetFloat(shaderProp.Name, shaderProp.DefaultFloat);
                                isChanged |= EditorGUI.FloatField(shaderProp.Label, shaderProp.Tooltip, ref value);
                                material.SetFloat(shaderProp.Name, value);
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
                                string textureAssetPath = material.GetTextureAssetPath(shaderProp.Name);
                                isChanged |= EditorGUI.TextField(shaderProp.Label, shaderProp.Tooltip, ref textureAssetPath);

                                Texture? texture = AssetManager.Load<Texture>(textureAssetPath);
                                material.SetTexture(shaderProp.Name, textureAssetPath, texture);
                                break;
                            }
                    }
                }
            }

            showApplyRevertButtons = true;
            return isChanged;
        }
    }
}
