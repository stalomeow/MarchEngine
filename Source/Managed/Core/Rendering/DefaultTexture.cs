using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    public enum DefaultTexture
    {
        Black, // RGBA: 0, 0, 0, 1
        White, // RGBA: 1, 1, 1, 1
        Bump,  // RGBA: 0.5, 0.5, 1, 1
        Gray,  // RGBA: 0.5, 0.5, 0.5, 1
        Red,   // RGBA: 1, 0, 0, 1

        // Alias
        Grey = Gray,
    }

    public partial class Texture
    {
        private record struct DefaultTexQuery(DefaultTexture Texture, TextureDimension Dimension);

        private static readonly Dictionary<DefaultTexQuery, ExternalTexture> s_Textures = new();

        public static Texture GetDefault(DefaultTexture @default, TextureDimension dimension = TextureDimension.Tex2D)
        {
            var query = new DefaultTexQuery(@default, dimension);

            if (!s_Textures.TryGetValue(query, out ExternalTexture? texture))
            {
                texture = new ExternalTexture();
                Load(texture, @default, dimension);
                s_Textures.Add(query, texture);
            }

            return texture;
        }

        [UnmanagedCallersOnly]
        private static nint NativeGetDefault(DefaultTexture texture, TextureDimension dimension)
        {
            return GetDefault(texture, dimension).NativePtr;
        }

        internal static void DestroyDefaults()
        {
            foreach (KeyValuePair<DefaultTexQuery, ExternalTexture> kv in s_Textures)
            {
                kv.Value.Dispose();
            }

            s_Textures.Clear();
        }

        private static void Load(ExternalTexture texture, DefaultTexture @default, TextureDimension dimension)
        {
            Span<byte> pixels = stackalloc byte[24];
            int size = FillDefaultPixels(@default, dimension, pixels);

            TextureDesc desc = GetDefaultDesc(@default, dimension);
            string name = GetDefaultName(@default, dimension);

            texture.LoadFromPixels(name, in desc, pixels[..size], mipLevels: 1);
        }

        private static TextureDesc GetDefaultDesc(DefaultTexture texture, TextureDimension dimension) => new()
        {
            Format = TextureFormat.R8G8B8A8_UNorm,
            Flags = (texture == DefaultTexture.Bump) ? TextureFlags.None : TextureFlags.SRGB,

            Dimension = dimension,
            Width = 1,
            Height = 1,
            DepthOrArraySize = 1,
            MSAASamples = 1,

            Filter = TextureFilterMode.Point,
            Wrap = TextureWrapMode.Clamp,
            MipmapBias = 0.0f,
        };

        private static string GetDefaultName(DefaultTexture texture, TextureDimension dimension)
        {
            string dimName = dimension switch
            {
                TextureDimension.Tex2D => "2D",
                TextureDimension.Tex3D => "3D",
                TextureDimension.Cube => "Cube",
                TextureDimension.Tex2DArray => "2DArray",
                TextureDimension.CubeArray => "CubeArray",
                _ => throw new ArgumentOutOfRangeException(nameof(dimension)),
            };

            return $"Default{dimName} ({texture})";
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="texture"></param>
        /// <param name="dimension"></param>
        /// <param name="pixels">需要保证大小至少为 24</param>
        /// <returns></returns>
        /// <exception cref="ArgumentOutOfRangeException"></exception>
        /// <exception cref="NotSupportedException"></exception>
        private static int FillDefaultPixels(DefaultTexture texture, TextureDimension dimension, Span<byte> pixels)
        {
            int faceCount = dimension is (TextureDimension.Cube or TextureDimension.CubeArray) ? 6 : 1;
            int totalSize = faceCount * 4;

            if (pixels.Length < totalSize)
            {
                throw new ArgumentOutOfRangeException(nameof(pixels), $"The size of pixels must be at least {totalSize}.");
            }

            ReadOnlySpan<byte> color = texture switch
            {
                DefaultTexture.Black => [0x00, 0x00, 0x00, 0xFF],
                DefaultTexture.White => [0xFF, 0xFF, 0xFF, 0xFF],
                DefaultTexture.Bump => [0x80, 0x80, 0xFF, 0xFF],
                DefaultTexture.Gray => [0x80, 0x80, 0x80, 0xFF],
                DefaultTexture.Red => [0xFF, 0x00, 0x00, 0xFF],
                _ => throw new NotSupportedException("Unsupported default texture type: " + texture.ToString()),
            };

            // 每个面一个像素
            for (int i = 0; i < faceCount; i++)
            {
                color.CopyTo(pixels.Slice(i * 4, 4));
            }

            return totalSize;
        }
    }
}
