using March.Core.Binding;
using Newtonsoft.Json;
using System.Numerics;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
    public partial class Material : NativeEngineObject
    {
        [JsonIgnore] private Shader? m_Shader = null;
        [JsonProperty] private Dictionary<string, int> m_Ints = [];
        [JsonProperty] private Dictionary<string, float> m_Floats = [];
        [JsonProperty] private Dictionary<string, Vector4> m_Vectors = [];
        [JsonProperty] private Dictionary<string, Color> m_Colors = [];
        [JsonProperty] private Dictionary<string, Texture> m_Textures = [];

        [JsonProperty]
        public Shader? Shader
        {
            get => m_Shader;
            set
            {
                m_Shader = value;
                Material_SetShader(NativePtr, value?.NativePtr ?? nint.Zero);
            }
        }

        public Material() : base(Material_New()) { }

        protected override void Dispose(bool disposing)
        {
            Material_Delete(NativePtr);
        }

        [OnDeserializing]
        private void OnDeserializingCallback(StreamingContext context)
        {
            Material_Reset(NativePtr); // 避免 overwrite 后还有旧数据
        }

        [OnDeserialized]
        private void OnDeserializedCallback(StreamingContext context)
        {
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

            foreach (KeyValuePair<string, Texture> kvp in m_Textures)
            {
                using NativeString n = kvp.Key;
                Material_SetTexture(NativePtr, n.Data, kvp.Value.NativePtr);
            }
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
        /// <param name="value">如果为 <c>null</c> 则删除</param>
        public void SetTexture(string name, Texture? value)
        {
            if (value == null)
            {
                m_Textures.Remove(name);
            }
            else
            {
                m_Textures[name] = value;
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

        public Texture? GetTexture(string name, Texture? defaultValue = default)
        {
            return m_Textures.TryGetValue(name, out Texture? value) ? value : defaultValue;
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
