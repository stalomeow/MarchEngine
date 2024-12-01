using March.Core.Interop;
using Newtonsoft.Json;
using System.Runtime.Serialization;

namespace March.Core.Rendering
{
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
