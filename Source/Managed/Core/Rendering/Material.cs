using March.Core.Interop;
using Newtonsoft.Json;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
    public sealed unsafe partial class Material : NativeMarchObject
    {
        private Shader? m_Shader = null;
        private readonly Dictionary<string, Texture> m_NonNullTextures = [];

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
            // 防止 overwrite 后有旧属性残留
            Reset();
        }

        public void Reset()
        {
            m_Shader = null;
            m_NonNullTextures.Clear();
            Material_Reset(NativePtr);
        }

        public void SetInt(string name, int value)
        {
            using NativeString n = name;
            Material_SetInt(NativePtr, n.Data, value);
        }

        public void SetFloat(string name, float value)
        {
            using NativeString n = name;
            Material_SetFloat(NativePtr, n.Data, value);
        }

        public void SetVector(string name, Vector4 value)
        {
            using NativeString n = name;
            Material_SetVector(NativePtr, n.Data, value);
        }

        public void SetColor(string name, Color value)
        {
            using NativeString n = name;
            Material_SetColor(NativePtr, n.Data, value);
        }

        public void SetTexture(string name, Texture? value)
        {
            if (value == null)
            {
                m_NonNullTextures.Remove(name);
            }
            else
            {
                m_NonNullTextures[name] = value;
            }

            using NativeString n = name;
            Material_SetTexture(NativePtr, n.Data, value?.NativePtr ?? nint.Zero);
        }

        public bool TryGetInt(string name, out int value)
        {
            using NativeString n = name;

            int v = default;
            bool ret = Material_GetInt(NativePtr, n.Data, &v);
            value = v;
            return ret;
        }

        public bool TryGetFloat(string name, out float value)
        {
            using NativeString n = name;

            float v = default;
            bool ret = Material_GetFloat(NativePtr, n.Data, &v);
            value = v;
            return ret;
        }

        public bool TryGetVector(string name, out Vector4 value)
        {
            using NativeString n = name;

            Vector4 v = default;
            bool ret = Material_GetVector(NativePtr, n.Data, &v);
            value = v;
            return ret;
        }

        public bool TryGetColor(string name, out Color value)
        {
            using NativeString n = name;

            Color v = default;
            bool ret = Material_GetColor(NativePtr, n.Data, &v);
            value = v;
            return ret;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <param name="value">即使返回值为 <c>true</c> 该值也可能为 <c>null</c>；返回值只代表有没有这个属性，属性值可以为 <c>null</c></param>
        /// <returns></returns>
        public bool TryGetTexture(string name, out Texture? value)
        {
            if (m_NonNullTextures.TryGetValue(name, out value))
            {
                return true;
            }

            value = null;
            using NativeString n = name;
            return Material_HasTextureProperty(NativePtr, n.Data);
        }

        public int GetIntOrDefault(string name, int defaultValue = default)
        {
            return TryGetInt(name, out int value) ? value : defaultValue;
        }

        public float GetFloatOrDefault(string name, float defaultValue = default)
        {
            return TryGetFloat(name, out float value) ? value : defaultValue;
        }

        public Vector4 GetVectorOrDefault(string name, Vector4 defaultValue = default)
        {
            return TryGetVector(name, out Vector4 value) ? value : defaultValue;
        }

        public Color GetColorOrDefault(string name, Color defaultValue = default)
        {
            return TryGetColor(name, out Color value) ? value : defaultValue;
        }

        public Texture? GetTextureOrDefault(string name, Texture? defaultValue = default)
        {
            return TryGetTexture(name, out Texture? value) ? value : defaultValue;
        }

        public int MustGetInt(string name)
        {
            return TryGetInt(name, out int value) ? value : throw new KeyNotFoundException("Material property not found: " + name);
        }

        public float MustGetFloat(string name)
        {
            return TryGetFloat(name, out float value) ? value : throw new KeyNotFoundException("Material property not found: " + name);
        }

        public Vector4 MustGetVector(string name)
        {
            return TryGetVector(name, out Vector4 value) ? value : throw new KeyNotFoundException("Material property not found: " + name);
        }

        public Color MustGetColor(string name)
        {
            return TryGetColor(name, out Color value) ? value : throw new KeyNotFoundException("Material property not found: " + name);
        }

        public Texture? MustGetTexture(string name)
        {
            return TryGetTexture(name, out Texture? value) ? value : throw new KeyNotFoundException("Material property not found: " + name);
        }

        public void EnableKeyword(string keyword)
        {
            using NativeString k = keyword;
            Material_EnableKeyword(NativePtr, k.Data);
        }

        public void DisableKeyword(string keyword)
        {
            using NativeString k = keyword;
            Material_DisableKeyword(NativePtr, k.Data);
        }

        public void SetKeyword(string keyword, bool value)
        {
            using NativeString k = keyword;
            Material_SetKeyword(NativePtr, k.Data, value);
        }

        #region Serialization

        [StructLayout(LayoutKind.Sequential)]
        private struct NativeProperty<T> where T : unmanaged
        {
            public nint Name;
            public T Value;
        }

        [JsonProperty("m_Ints")]
        private Dictionary<string, int> IntProperties
        {
            get
            {
                using var props = (NativeArray<NativeProperty<int>>)Material_GetAllInts(NativePtr);
                var result = new Dictionary<string, int>();

                for (int i = 0; i < props.Length; i++)
                {
                    ref NativeProperty<int> p = ref props[i];
                    result.Add(NativeString.GetAndFree(p.Name), p.Value);
                }

                return result;
            }

            set
            {
                foreach (KeyValuePair<string, int> kvp in value)
                {
                    SetInt(kvp.Key, kvp.Value);
                }
            }
        }

        [JsonProperty("m_Floats")]
        private Dictionary<string, float> FloatProperties
        {
            get
            {
                using var props = (NativeArray<NativeProperty<float>>)Material_GetAllFloats(NativePtr);
                var result = new Dictionary<string, float>();

                for (int i = 0; i < props.Length; i++)
                {
                    ref NativeProperty<float> p = ref props[i];
                    result.Add(NativeString.GetAndFree(p.Name), p.Value);
                }

                return result;
            }

            set
            {
                foreach (KeyValuePair<string, float> kvp in value)
                {
                    SetFloat(kvp.Key, kvp.Value);
                }
            }
        }

        [JsonProperty("m_Vectors")]
        private Dictionary<string, Vector4> VectorProperties
        {
            get
            {
                using var props = (NativeArray<NativeProperty<Vector4>>)Material_GetAllVectors(NativePtr);
                var result = new Dictionary<string, Vector4>();

                for (int i = 0; i < props.Length; i++)
                {
                    ref NativeProperty<Vector4> p = ref props[i];
                    result.Add(NativeString.GetAndFree(p.Name), p.Value);
                }

                return result;
            }

            set
            {
                foreach (KeyValuePair<string, Vector4> kvp in value)
                {
                    SetVector(kvp.Key, kvp.Value);
                }
            }
        }

        [JsonProperty("m_Colors")]
        private Dictionary<string, Color> ColorProperties
        {
            get
            {
                using var props = (NativeArray<NativeProperty<Color>>)Material_GetAllColors(NativePtr);
                var result = new Dictionary<string, Color>();

                for (int i = 0; i < props.Length; i++)
                {
                    ref NativeProperty<Color> p = ref props[i];
                    result.Add(NativeString.GetAndFree(p.Name), p.Value);
                }

                return result;
            }

            set
            {
                foreach (KeyValuePair<string, Color> kvp in value)
                {
                    SetColor(kvp.Key, kvp.Value);
                }
            }
        }

        [JsonProperty("m_Textures")]
        private Dictionary<string, Texture?> TextureProperties
        {
            get
            {
                using var props = (NativeArray<NativeProperty<nint>>)Material_GetAllTextures(NativePtr);
                var result = new Dictionary<string, Texture?>();

                for (int i = 0; i < props.Length; i++)
                {
                    ref NativeProperty<nint> p = ref props[i];
                    string name = NativeString.GetAndFree(p.Name);
                    result.Add(name, m_NonNullTextures.GetValueOrDefault(name));
                }

                return result;
            }

            set
            {
                foreach (KeyValuePair<string, Texture?> kvp in value)
                {
                    SetTexture(kvp.Key, kvp.Value);
                }
            }
        }

        [JsonProperty("m_Keywords")]
        private string[] Keywords
        {
            get
            {
                using var ks = (NativeArray<nint>)Material_GetAllKeywords(NativePtr);
                return ks.ConvertValue((in nint s) => NativeString.GetAndFree(s));
            }

            set
            {
                foreach (string keyword in value)
                {
                    EnableKeyword(keyword);
                }
            }
        }

        #endregion

        #region Native

        [NativeMethod]
        private static partial nint Material_New();

        [NativeMethod]
        private static partial void Material_Delete(nint pMaterial);

        [NativeMethod]
        private static partial void Material_Reset(nint pMaterial);

        [NativeMethod]
        private static partial void Material_SetShader(nint pMaterial, nint pShader);

        [NativeMethod]
        private static partial void Material_SetInt(nint pMaterial, nint name, int value);

        [NativeMethod]
        private static partial void Material_SetFloat(nint pMaterial, nint name, float value);

        [NativeMethod]
        private static partial void Material_SetVector(nint pMaterial, nint name, Vector4 value);

        [NativeMethod]
        private static partial void Material_SetColor(nint pMaterial, nint name, Color value);

        [NativeMethod]
        private static partial void Material_SetTexture(nint pMaterial, nint name, nint pTexture);

        [NativeMethod]
        private static partial bool Material_GetInt(nint pMaterial, nint name, int* outValue);

        [NativeMethod]
        private static partial bool Material_GetFloat(nint pMaterial, nint name, float* outValue);

        [NativeMethod]
        private static partial bool Material_GetVector(nint pMaterial, nint name, Vector4* outValue);

        [NativeMethod]
        private static partial bool Material_GetColor(nint pMaterial, nint name, Color* outValue);

        [NativeMethod]
        private static partial bool Material_HasTextureProperty(nint pMaterial, nint name);

        [NativeMethod]
        private static partial nint Material_GetAllInts(nint pMaterial);

        [NativeMethod]
        private static partial nint Material_GetAllFloats(nint pMaterial);

        [NativeMethod]
        private static partial nint Material_GetAllVectors(nint pMaterial);

        [NativeMethod]
        private static partial nint Material_GetAllColors(nint pMaterial);

        [NativeMethod]
        private static partial nint Material_GetAllTextures(nint pMaterial);

        [NativeMethod]
        private static partial void Material_EnableKeyword(nint pMaterial, nint keyword);

        [NativeMethod]
        private static partial void Material_DisableKeyword(nint pMaterial, nint keyword);

        [NativeMethod]
        private static partial void Material_SetKeyword(nint pMaterial, nint keyword, bool value);

        [NativeMethod]
        private static partial nint Material_GetAllKeywords(nint pMaterial);

        #endregion
    }
}
