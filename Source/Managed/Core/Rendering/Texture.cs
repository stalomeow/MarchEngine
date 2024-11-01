using March.Core.Interop;
using Newtonsoft.Json;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
    public enum FilterMode : int
    {
        Point = 0,
        Bilinear = 1,
        Trilinear = 2,
    }

    public enum WrapMode : int
    {
        Repeat = 0,
        Clamp = 1,
        Mirror = 2,
    }

    public enum Texture2DSourceType
    {
        DDS = 0,
        WIC = 1,
    }

    public partial class Texture : NativeMarchObject
    {
        [JsonProperty]
        private string m_SerializedName = "Texture";

        [JsonProperty]
        private byte[] m_SourceData = [];

        [JsonProperty]
        internal Texture2DSourceType SourceType { get; set; }

        [JsonProperty]
        public bool IsSRGB
        {
            get => Texture_GetIsSRGB(NativePtr);
            set => Texture_SetIsSRGB(NativePtr, value);
        }

        [JsonProperty]
        public FilterMode Filter
        {
            get => Texture_GetFilterMode(NativePtr);
            set => Texture_SetFilterMode(NativePtr, value);
        }

        [JsonProperty]
        public WrapMode Wrap
        {
            get => Texture_GetWrapMode(NativePtr);
            set => Texture_SetWrapMode(NativePtr, value);
        }

        public Texture() : base(Texture_New()) { }

        protected override void Dispose(bool disposing)
        {
            Texture_Delete(NativePtr);
        }

        public unsafe void LoadFromSource(string name, byte[] source)
        {
            using NativeString n = name;

            fixed (byte* p = source)
            {
                Texture_LoadFromSource(NativePtr, n.Data, SourceType, (nint)p, source.Length);
            }
        }

        public void SetSerializationData(string name, byte[] source)
        {
            m_SerializedName = name;
            m_SourceData = source;
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            LoadFromSource(m_SerializedName, m_SourceData);
            SetSerializationData(string.Empty, []);
        }

        #region Native

        [NativeFunction]
        private static partial nint Texture_New();

        [NativeFunction]
        private static partial void Texture_Delete(nint pTexture);

        [NativeFunction]
        private static partial void Texture_LoadFromSource(nint pTexture, nint name, Texture2DSourceType sourceType, nint pSource, int size);

        [NativeFunction]
        private static partial void Texture_SetFilterMode(nint pTexture, FilterMode filterMode);

        [NativeFunction]
        private static partial void Texture_SetWrapMode(nint pTexture, WrapMode wrapMode);

        [NativeFunction]
        private static partial FilterMode Texture_GetFilterMode(nint pTexture);

        [NativeFunction]
        private static partial WrapMode Texture_GetWrapMode(nint pTexture);

        [NativeFunction]
        private static partial bool Texture_GetIsSRGB(nint pTexture);

        [NativeFunction]
        private static partial void Texture_SetIsSRGB(nint pTexture, bool sRGB);

        #endregion
    }
}
