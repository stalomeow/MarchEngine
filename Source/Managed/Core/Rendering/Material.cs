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
                SetNativeShader(value);
            }
        }

        public Material() : base(New()) { }

        protected override void Dispose(bool disposing)
        {
            Delete();
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
            ResetNative();
        }

        [NativeMethod]
        public partial void SetInt(string name, int value);

        [NativeMethod]
        public partial void SetFloat(string name, float value);

        [NativeMethod]
        public partial void SetVector(string name, Vector4 value);

        [NativeMethod]
        public partial void SetColor(string name, Color value);

        [NativeMethod("SetTexture")]
        private partial void SetNativeTexture(string name, Texture? texture);

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

            SetNativeTexture(name, value);
        }

        [NativeMethod("GetInt")]
        public partial bool TryGetInt(string name, out int value);

        [NativeMethod("GetFloat")]
        public partial bool TryGetFloat(string name, out float value);

        [NativeMethod("GetVector")]
        public partial bool TryGetVector(string name, out Vector4 value);

        [NativeMethod("GetColor")]
        public partial bool TryGetColor(string name, out Color value);

        [NativeMethod]
        private partial bool HasTextureProperty(string name);

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
            return HasTextureProperty(name);
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

        [NativeMethod]
        public partial void EnableKeyword(string keyword);

        [NativeMethod]
        public partial void DisableKeyword(string keyword);

        [NativeMethod]
        public partial void SetKeyword(string keyword, bool value);

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
                using var props = (NativeArray<NativeProperty<int>>)GetAllInts();
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
                using var props = (NativeArray<NativeProperty<float>>)GetAllFloats();
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
                using var props = (NativeArray<NativeProperty<Vector4>>)GetAllVectors();
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
                using var props = (NativeArray<NativeProperty<Color>>)GetAllColors();
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
                using var props = (NativeArray<NativeProperty<nint>>)GetAllTextures();
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
                using var ks = (NativeArray<nint>)GetAllKeywords();
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

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void Delete();

        [NativeMethod("Reset")]
        private partial void ResetNative();

        [NativeMethod("SetShader")]
        private partial void SetNativeShader(Shader? shader);

        [NativeMethod]
        private partial nint GetAllInts();

        [NativeMethod]
        private partial nint GetAllFloats();

        [NativeMethod]
        private partial nint GetAllVectors();

        [NativeMethod]
        private partial nint GetAllColors();

        [NativeMethod]
        private partial nint GetAllTextures();

        [NativeMethod]
        private partial nint GetAllKeywords();
    }
}
