using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    public enum DefaultTexture
    {
        Black, // RGBA: 0, 0, 0, 1
        White, // RGBA: 1, 1, 1, 1
        Bump,  // RGBA: 0.5, 0.5, 1, 1
    }

    public partial class Texture
    {
        private static readonly Dictionary<DefaultTexture, ExternalTexture> s_Textures = new();

        public static Texture GetDefault(DefaultTexture @default)
        {
            if (!s_Textures.TryGetValue(@default, out ExternalTexture? texture))
            {
                texture = new ExternalTexture();
                Load(texture, @default);
                s_Textures.Add(@default, texture);
            }

            return texture;
        }

        [UnmanagedCallersOnly]
        private static nint NativeGetDefault(DefaultTexture texture)
        {
            return GetDefault(texture).NativePtr;
        }

        internal static void DestroyDefaults()
        {
            foreach (KeyValuePair<DefaultTexture, ExternalTexture> kv in s_Textures)
            {
                kv.Value.Dispose();
            }

            s_Textures.Clear();
        }

        private static void Load(ExternalTexture texture, DefaultTexture @default)
        {
            switch (@default)
            {
                case DefaultTexture.Black:
                    LoadBlack(texture);
                    break;

                case DefaultTexture.White:
                    LoadWhite(texture);
                    break;

                case DefaultTexture.Bump:
                    LoadBump(texture);
                    break;

                default:
                    throw new NotSupportedException("Unsupported default texture type: " + @default.ToString());
            }
        }

        private static void LoadBlack(ExternalTexture texture)
        {
            TextureDesc desc = new()
            {
                Format = TextureFormat.R8G8B8A8_UNorm,
                Flags = TextureFlags.SRGB,

                Dimension = TextureDimension.Tex2D,
                Width = 1,
                Height = 1,
                DepthOrArraySize = 1,
                MSAASamples = 1,

                Filter = TextureFilterMode.Point,
                Wrap = TextureWrapMode.Clamp,
                MipmapBias = 0.0f,
            };

            ReadOnlySpan<byte> pixels = [0x00, 0x00, 0x00, 0xFF];
            texture.LoadFromPixels("DefaultBlackTexture", in desc, pixels, mipLevels: 1);
        }

        private static void LoadWhite(ExternalTexture texture)
        {
            TextureDesc desc = new()
            {
                Format = TextureFormat.R8G8B8A8_UNorm,
                Flags = TextureFlags.SRGB,

                Dimension = TextureDimension.Tex2D,
                Width = 1,
                Height = 1,
                DepthOrArraySize = 1,
                MSAASamples = 1,

                Filter = TextureFilterMode.Point,
                Wrap = TextureWrapMode.Clamp,
                MipmapBias = 0.0f,
            };

            ReadOnlySpan<byte> pixels = [0xFF, 0xFF, 0xFF, 0xFF];
            texture.LoadFromPixels("DefaultWhiteTexture", in desc, pixels, mipLevels: 1);
        }

        private static void LoadBump(ExternalTexture texture)
        {
            TextureDesc desc = new()
            {
                Format = TextureFormat.R8G8B8A8_UNorm,
                Flags = TextureFlags.None, // No sRGB

                Dimension = TextureDimension.Tex2D,
                Width = 1,
                Height = 1,
                DepthOrArraySize = 1,
                MSAASamples = 1,

                Filter = TextureFilterMode.Point,
                Wrap = TextureWrapMode.Clamp,
                MipmapBias = 0.0f,
            };

            ReadOnlySpan<byte> pixels = [0x80, 0x80, 0xFF, 0xFF];
            texture.LoadFromPixels("DefaultBumpTexture", in desc, pixels, mipLevels: 1);
        }
    }
}
