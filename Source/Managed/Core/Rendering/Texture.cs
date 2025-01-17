using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.Runtime.InteropServices;

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
        R8G8B8A8_UInt,
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

    [Flags]
    public enum TextureFlags
    {
        None = 0,

        [InspectorName("sRGB")]
        SRGB = 1 << 0,

        Mipmaps = 1 << 1,

        [InspectorName("Unordered Access")]
        UnorderedAccess = 1 << 2,

        [HideInInspector]
        SwapChain = 1 << 3,
    }

    public enum TextureDimension
    {
        [InspectorName("2D")] Tex2D,
        [InspectorName("3D")] Tex3D,
        [InspectorName("Cube")] Cube,
        [InspectorName("2D Array")] Tex2DArray,
        [InspectorName("Cube Array")] CubeArray,
    }

    public enum TextureFilterMode
    {
        Point,
        Bilinear,
        Trilinear,

        // Shadow 使用 Comparison Sampler，和其他的 Sampler 不一样
        [HideInInspector]
        Shadow,

        [InspectorName("Anisotropic 1")] Anisotropic1,
        [InspectorName("Anisotropic 2")] Anisotropic2,
        [InspectorName("Anisotropic 3")] Anisotropic3,
        [InspectorName("Anisotropic 4")] Anisotropic4,
        [InspectorName("Anisotropic 5")] Anisotropic5,
        [InspectorName("Anisotropic 6")] Anisotropic6,
        [InspectorName("Anisotropic 7")] Anisotropic7,
        [InspectorName("Anisotropic 8")] Anisotropic8,
        [InspectorName("Anisotropic 9")] Anisotropic9,
        [InspectorName("Anisotropic 10")] Anisotropic10,
        [InspectorName("Anisotropic 11")] Anisotropic11,
        [InspectorName("Anisotropic 12")] Anisotropic12,
        [InspectorName("Anisotropic 13")] Anisotropic13,
        [InspectorName("Anisotropic 14")] Anisotropic14,
        [InspectorName("Anisotropic 15")] Anisotropic15,
        [InspectorName("Anisotropic 16")] Anisotropic16,
    }

    public enum TextureWrapMode
    {
        Repeat,
        Clamp,
        Mirror,

        [InspectorName("Mirror Once")]
        MirrorOnce,
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct TextureDesc
    {
        public TextureFormat Format;
        public TextureFlags Flags;

        public TextureDimension Dimension;
        public uint Width;
        public uint Height;
        public uint DepthOrArraySize; // Cubemap 时为 1，CubemapArray 时为 Cubemap 的数量，都不用乘 6
        public uint MSAASamples;

        public TextureFilterMode Filter;
        public TextureWrapMode Wrap;
        public float MipmapBias;
    }

    [NativeTypeName("GfxTexture")]
    public abstract partial class Texture(nint nativePtr) : NativeMarchObject(nativePtr)
    {
        [NativeProperty]
        public partial uint MipLevels { get; }

        [NativeProperty]
        public partial TextureDesc Desc { get; }

        [NativeProperty]
        public partial bool AllowRendering { get; }
    }

    internal sealed class ExternalTextureData
    {
        public string Name = string.Empty;
        public TextureDesc Desc;
        public byte[] Pixels = [];
        public uint MipLevels;
    }

    public enum TextureCompression
    {
        [InspectorName("Normal Quality")] NormalQuality,
        [InspectorName("High Quality")] HighQuality,
        [InspectorName("Low Quality")] LowQuality,
        None,
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct LoadTextureFileArgs
    {
        public TextureFlags Flags;
        public TextureFilterMode Filter;
        public TextureWrapMode Wrap;
        public float MipmapBias;
        public TextureCompression Compression;
    }

    [NativeTypeName("GfxExternalTexture")]
    internal unsafe partial class ExternalTexture : Texture
    {
        public ExternalTexture() : base(New()) { }

        public byte[] Pixels
        {
            get
            {
                byte[] pixels = new byte[PixelsSize];

                fixed (void* p = pixels)
                {
                    Buffer.MemoryCopy(GetPixelsData(), p, pixels.LongLength, pixels.LongLength);
                }

                return pixels;
            }
        }

        [JsonProperty("Data")]
        private ExternalTextureData DataSerializationOnly
        {
            get => new()
            {
                Name = Name,
                Desc = Desc,
                Pixels = Pixels,
                MipLevels = MipLevels,
            };

            set
            {
                fixed (void* p = value.Pixels)
                {
                    LoadFromPixels(value.Name, in value.Desc, p, value.Pixels.LongLength, value.MipLevels);
                }
            }
        }

        public void LoadFromPixels(string name, in TextureDesc desc, ReadOnlySpan<byte> pixels, uint mipLevels)
        {
            fixed (void* p = pixels)
            {
                LoadFromPixels(name, in desc, p, pixels.Length, mipLevels);
            }
        }

        [NativeProperty]
        public partial string Name { get; }

        /// <summary>
        /// 所有像素占用的总字节数
        /// </summary>
        [NativeProperty]
        public partial long PixelsSize { get; }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void* GetPixelsData();

        [NativeMethod]
        private partial void LoadFromPixels(string name, in TextureDesc desc, void* pixelsData, long pixelsSize, uint mipLevels);

        [NativeMethod]
        public partial void LoadFromFile(string name, string filePath, in LoadTextureFileArgs args);
    }
}
