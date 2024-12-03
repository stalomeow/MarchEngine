using March.Core.Interop;
using Newtonsoft.Json;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
    public enum TextureFormat
    {
        R32G32B32A32_Float,
        R32G32B32A32_UInt,
        R32G32B32A32_SInt,
        R32G32B32_Float,
        R32G32B32_UInt,
        R32G32B32_SInt,
        R32G32_Float,
        R32G32_UInt,
        R32G32_SInt,
        R32_Float,
        R32_UInt,
        R32_SInt,

        R16G16B16A16_Float,
        R16G16B16A16_UNorm,
        R16G16B16A16_UInt,
        R16G16B16A16_SNorm,
        R16G16B16A16_SInt,
        R16G16_Float,
        R16G16_UNorm,
        R16G16_UInt,
        R16G16_SNorm,
        R16G16_SInt,
        R16_Float,
        R16_UNorm,
        R16_UInt,
        R16_SNorm,
        R16_SInt,

        R8G8B8A8_UNorm,
        R8G8B8A8_UINT,
        R8G8B8A8_SNorm,
        R8G8B8A8_SInt,
        R8G8_UNorm,
        R8G8_UInt,
        R8G8_SNorm,
        R8G8_SInt,
        R8_UNorm,
        R8_UInt,
        R8_SNorm,
        R8_SInt,
        A8_UNorm,

        R11G11B10_Float,
        R10G10B10A2_UNorm,
        R10G10B10A2_UInt,

        B5G6R5_UNorm,
        B5G5R5A1_UNorm,
        B8G8R8A8_UNorm,
        B8G8R8_UNorm,
        B4G4R4A4_UNorm,

        BC1_UNorm,
        BC2_UNorm,
        BC3_UNorm,
        BC4_UNorm,
        BC4_SNorm,
        BC5_UNorm,
        BC5_SNorm,
        BC6H_UF16,
        BC6H_SF16,
        BC7_UNorm,

        D32_Float_S8_UInt,
        D32_Float,
        D24_UNorm_S8_UInt,
        D16_UNorm,
    }

    public enum FilterMode
    {
        Point = 0,
        Bilinear = 1,
        Trilinear = 2,
    }

    public enum WrapMode
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

    public enum TextureDimension
    {
        Unknown,
        _2D,
        _2DArray,
        _3D,
        Cube,
        CubeArray,
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
        [NativeProperty]
        public partial bool IsSRGB { get; set; }

        [JsonProperty]
        [NativeProperty("FilterMode")]
        public partial FilterMode Filter { get; set; }

        [JsonProperty]
        [NativeProperty("WrapMode")]
        public partial WrapMode Wrap { get; set; }

        public Texture() : base(New()) { }

        protected override void Dispose(bool disposing)
        {
            Delete();
        }

        public unsafe void LoadFromSource(string name, byte[] source)
        {
            fixed (byte* p = source)
            {
                LoadFromSource(name, SourceType, (nint)p, source.Length);
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

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void Delete();

        [NativeMethod]
        private partial void LoadFromSource(string name, Texture2DSourceType sourceType, nint pSource, int size);
    }
}
