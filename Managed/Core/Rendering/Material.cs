using DX12Demo.Core.Binding;
using Newtonsoft.Json;
using System.Numerics;
using System.Runtime.Serialization;

namespace DX12Demo.Core.Rendering
{
    public partial class Material : NativeEngineObject
    {
        private struct TextureReference
        {
            public Texture Tex;
            public int RefCount;
        }

        [JsonProperty] private string m_ShaderPath = string.Empty;
        [JsonProperty] private Dictionary<string, int> m_Ints = [];
        [JsonProperty] private Dictionary<string, float> m_Floats = [];
        [JsonProperty] private Dictionary<string, Vector4> m_Vectors = [];
        [JsonProperty] private Dictionary<string, Color> m_Colors = [];
        [JsonProperty] private Dictionary<string, string> m_Textures = [];

        [JsonIgnore] private Shader? m_Shader = null;
        [JsonIgnore] private Dictionary<string, TextureReference> m_TextureReferences = [];

        public Material() : base(Material_New()) { }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                m_TextureReferences.Clear();
            }

            Material_Delete(NativePtr);
        }

        [OnDeserialized]
        private void OnDeserializedCallback(StreamingContext context)
        {
            Material_Reset(NativePtr); // 避免 overwrite 后还有旧数据

            m_Shader = AssetManager.Load<Shader>(m_ShaderPath);

            if (m_Shader != null)
            {
                Material_SetShader(NativePtr, m_Shader.NativePtr);
            }

            foreach (KeyValuePair<string, int> kvp in m_Ints)
            {
                using NativeString n = kvp.Key;
                Material_SetInt(NativePtr, n.Data, kvp.Value);
            }

            foreach (KeyValuePair<string, float> kvp in m_Floats)
            {
                using NativeString n = kvp.Key;
                Material_SetFloat(NativePtr, n.Data, kvp.Value);
            }

            foreach (KeyValuePair<string, Vector4> kvp in m_Vectors)
            {
                using NativeString n = kvp.Key;
                Material_SetVector(NativePtr, n.Data, kvp.Value);
            }

            foreach (KeyValuePair<string, Color> kvp in m_Colors)
            {
                using NativeString n = kvp.Key;
                Material_SetVector(NativePtr, n.Data, new Vector4(kvp.Value.R, kvp.Value.G, kvp.Value.B, kvp.Value.A));
            }

            m_TextureReferences.Clear();

            foreach (KeyValuePair<string, string> kvp in m_Textures)
            {
                Texture? texture = AssetManager.Load<Texture>(kvp.Value);

                if (texture != null)
                {
                    AddTextureRef(kvp.Value, texture);

                    using NativeString n = kvp.Key;
                    Material_SetTexture(NativePtr, n.Data, texture.NativePtr);
                }
            }
        }

        private void AddTextureRef(string textureAssetPath, Texture texture)
        {
            if (!m_TextureReferences.TryGetValue(textureAssetPath, out TextureReference reference))
            {
                reference = new TextureReference { Tex = texture };
            }

            reference.RefCount++;
            m_TextureReferences[textureAssetPath] = reference;
        }

        private void RemoveTextureRef(string textureAssetPath)
        {
            if (!m_TextureReferences.TryGetValue(textureAssetPath, out TextureReference reference))
            {
                return;
            }

            reference.RefCount--;

            if (reference.RefCount <= 0)
            {
                m_TextureReferences.Remove(textureAssetPath);
            }
            else
            {
                m_TextureReferences[textureAssetPath] = reference;
            }
        }

        public Shader Shader => m_Shader!;

        public void SetShader(string shaderAssetPath, Shader shader)
        {
            m_ShaderPath = shaderAssetPath;
            m_Shader = shader;
        }

        public void SetInt(string name, int value)
        {
            m_Ints[name] = value;

            using NativeString n = name;
            Material_SetInt(NativePtr, n.Data, value);
        }

        public void SetFloat(string name, float value)
        {
            m_Floats[name] = value;

            using NativeString n = name;
            Material_SetFloat(NativePtr, n.Data, value);
        }

        public void SetVector(string name, Vector4 value)
        {
            m_Vectors[name] = value;

            using NativeString n = name;
            Material_SetVector(NativePtr, n.Data, value);
        }

        public void SetColor(string name, Color value)
        {
            m_Colors[name] = value;

            using NativeString n = name;
            Material_SetVector(NativePtr, n.Data, new Vector4(value.R, value.G, value.B, value.A));
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <param name="textureAssetPath"></param>
        /// <param name="value">如果为 <c>null</c> 则删除</param>
        public void SetTexture(string name, string textureAssetPath, Texture? value)
        {
            if (m_Textures.TryGetValue(name, out string? prevTextureAssetPath))
            {
                RemoveTextureRef(prevTextureAssetPath);
            }

            if (value == null)
            {
                m_Textures.Remove(name);
            }
            else
            {
                m_Textures[name] = textureAssetPath;
                AddTextureRef(textureAssetPath, value);
            }

            using NativeString n = name;
            Material_SetTexture(NativePtr, n.Data, value?.NativePtr ?? nint.Zero);
        }

        public int GetInt(string name, int defaultValue = default)
        {
            return m_Ints.GetValueOrDefault(name, defaultValue);
        }

        public float GetFloat(string name, float defaultValue = default)
        {
            return m_Floats.GetValueOrDefault(name, defaultValue);
        }

        public Vector4 GetVector(string name, Vector4 defaultValue = default)
        {
            return m_Vectors.GetValueOrDefault(name, defaultValue);
        }

        public Color GetColor(string name, Color defaultValue = default)
        {
            return m_Colors.GetValueOrDefault(name, defaultValue);
        }

        public string GetTextureAssetPath(string name, string defaultTextureAssetPath = "")
        {
            return m_Textures.GetValueOrDefault(name, defaultTextureAssetPath);
        }

        #region Native

        [NativeFunction]
        private static partial nint Material_New();

        [NativeFunction]
        private static partial void Material_Delete(nint pMaterial);

        [NativeFunction]
        private static partial void Material_Reset(nint pMaterial);

        [NativeFunction]
        private static partial void Material_SetShader(nint pMaterial, nint pShader);

        [NativeFunction]
        private static partial void Material_SetInt(nint pMaterial, nint name, int value);

        [NativeFunction]
        private static partial void Material_SetFloat(nint pMaterial, nint name, float value);

        [NativeFunction]
        private static partial void Material_SetVector(nint pMaterial, nint name, Vector4 value);

        [NativeFunction]
        private static partial void Material_SetTexture(nint pMaterial, nint name, nint pTexture);

        #endregion
    }
}
