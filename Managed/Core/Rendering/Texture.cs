using DX12Demo.Core.Binding;
using Newtonsoft.Json;
using System.Runtime.Serialization;

namespace DX12Demo.Core.Rendering
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

    public partial class Texture : EngineObject, IDisposable
    {
        [JsonProperty]
        private string m_SerializedName = "Texture";

        [JsonProperty]
        private byte[] m_SerializedDDSData = [];

        [JsonIgnore]
        private nint m_Texture;

        [JsonProperty]
        public FilterMode Filter
        {
            get => Texture_GetFilterMode(m_Texture);
            set => Texture_SetFilterMode(m_Texture, value);
        }

        [JsonProperty]
        public WrapMode Wrap
        {
            get => Texture_GetWrapMode(m_Texture);
            set => Texture_SetWrapMode(m_Texture, value);
        }

        public Texture()
        {
            m_Texture = Texture_New();
        }

        ~Texture() => DisposeImpl();

        protected virtual void DisposeImpl()
        {
            if (m_Texture != nint.Zero)
            {
                Texture_Delete(m_Texture);
                m_Texture = nint.Zero;
            }
        }

        public void Dispose()
        {
            DisposeImpl();
            GC.SuppressFinalize(this);
        }

        public unsafe void SetDDSData(string name, byte[] ddsData)
        {
            using NativeString n = name;

            fixed (byte* p = ddsData)
            {
                Texture_SetDDSData(m_Texture, n.Data, (nint)p, ddsData.Length);
            }
        }

        public void SetSerializationData(string name, byte[] ddsData)
        {
            m_SerializedName = name;
            m_SerializedDDSData = ddsData;
        }

        internal nint GetNativePointer() => m_Texture;

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            SetDDSData(m_SerializedName, m_SerializedDDSData);
            SetSerializationData(string.Empty, []);
        }

        [NativeFunction]
        private static partial nint Texture_New();

        [NativeFunction]
        private static partial void Texture_Delete(nint pTexture);

        [NativeFunction]
        private static partial void Texture_SetDDSData(nint pTexture, nint name, nint pSourceDDS, int size);

        [NativeFunction]
        private static partial void Texture_SetFilterMode(nint pTexture, FilterMode filterMode);

        [NativeFunction]
        private static partial void Texture_SetWrapMode(nint pTexture, WrapMode wrapMode);

        [NativeFunction]
        private static partial FilterMode Texture_GetFilterMode(nint pTexture);

        [NativeFunction]
        private static partial WrapMode Texture_GetWrapMode(nint pTexture);
    }
}
