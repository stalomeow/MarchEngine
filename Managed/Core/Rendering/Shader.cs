using DX12Demo.Core.Binding;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Serialization;
using System.Numerics;
using System.Runtime.InteropServices;

namespace DX12Demo.Core.Rendering
{
    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPropertyType : int
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderDefaultTexture : int
    {
        Black = 0,
        White = 1
    }

    [JsonObject(NamingStrategyType = typeof(ShaderJsonNamingStrategy))]
    internal class ShaderProperty
    {
        public string Name = string.Empty;
        public string Label = string.Empty;
        public string Tooltip = string.Empty;
        public ShaderPropertyType Type;

        public float DefaultFloat;
        public int DefaultInt;
        public Color DefaultColor;
        public Vector4 DefaultVector;
        public ShaderDefaultTexture DefaultTexture;
    }

    internal class ShaderPassConstantBuffer
    {
        public string Name = string.Empty;
        public uint ShaderRegister;
        public uint RegisterSpace;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint ShaderRegister;
            public uint RegisterSpace;
        }

        internal static ShaderPassConstantBuffer GetAndFree(ref Native native) => new()
        {
            Name = NativeString.GetAndFree(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
        };

        internal static void ToNative(ShaderPassConstantBuffer value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
        }

        internal static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal class ShaderPassSampler
    {
        public string Name = string.Empty;
        public uint ShaderRegister;
        public uint RegisterSpace;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint ShaderRegister;
            public uint RegisterSpace;
        }

        internal static ShaderPassSampler GetAndFree(ref Native native) => new()
        {
            Name = NativeString.GetAndFree(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
        };

        internal static void ToNative(ShaderPassSampler value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
        }

        internal static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal class ShaderPassMaterialProperty
    {
        public string Name = string.Empty;
        public uint Offset;
        public uint Size;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint Offset;
            public uint Size;
        }

        internal static ShaderPassMaterialProperty GetAndFree(ref Native native) => new()
        {
            Name = NativeString.GetAndFree(native.Name),
            Offset = native.Offset,
            Size = native.Size,
        };

        internal static void ToNative(ShaderPassMaterialProperty value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Offset = value.Offset;
            native.Size = value.Size;
        }

        internal static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal class ShaderPassTextureProperty
    {
        public string Name = string.Empty;
        public uint ShaderRegisterTexture;
        public uint RegisterSpaceTexture;
        public bool HasSampler;
        public uint ShaderRegisterSampler;
        public uint RegisterSpaceSampler;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint ShaderRegisterTexture;
            public uint RegisterSpaceTexture;
            public bool HasSampler;
            public uint ShaderRegisterSampler;
            public uint RegisterSpaceSampler;
        }

        internal static ShaderPassTextureProperty GetAndFree(ref Native native) => new()
        {
            Name = NativeString.GetAndFree(native.Name),
            ShaderRegisterTexture = native.ShaderRegisterTexture,
            RegisterSpaceTexture = native.RegisterSpaceTexture,
            HasSampler = native.HasSampler,
            ShaderRegisterSampler = native.ShaderRegisterSampler,
            RegisterSpaceSampler = native.RegisterSpaceSampler,
        };

        internal static void ToNative(ShaderPassTextureProperty value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegisterTexture = value.ShaderRegisterTexture;
            native.RegisterSpaceTexture = value.RegisterSpaceTexture;
            native.HasSampler = value.HasSampler;
            native.ShaderRegisterSampler = value.ShaderRegisterSampler;
            native.RegisterSpaceSampler = value.RegisterSpaceSampler;
        }

        internal static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassCullMode : int
    {
        Off = 0,
        Front = 1,
        Back = 2,
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassBlend : int
    {
        Zero = 0,
        One = 1,
        SrcColor = 2,
        InvSrcColor = 3,
        SrcAlpha = 4,
        InvSrcAlpha = 5,
        DestAlpha = 6,
        InvDestAlpha = 7,
        DestColor = 8,
        InvDestColor = 9,
        SrcAlphaSat = 10,
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassBlendOp : int
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    }

    [Flags]
    [JsonConverter(typeof(ShaderPassColorWriteMaskJsonConverter))]
    internal enum ShaderPassColorWriteMask : int
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    }

    [StructLayout(LayoutKind.Sequential)]
    [JsonObject(NamingStrategyType = typeof(ShaderJsonNamingStrategy))]
    internal struct ShaderPassBlendFormula
    {
        public ShaderPassBlend Src;
        public ShaderPassBlend Dest;
        public ShaderPassBlendOp Op;
    }

    [StructLayout(LayoutKind.Sequential)]
    [JsonObject(NamingStrategyType = typeof(ShaderJsonNamingStrategy))]
    internal struct ShaderPassBlendState
    {
        public bool Enable;
        public ShaderPassColorWriteMask WriteMask;
        public ShaderPassBlendFormula Rgb;
        public ShaderPassBlendFormula Alpha;
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassCompareFunc : int
    {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterEqual = 6,
        Always = 7,
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassDepthState
    {
        public bool Enable;
        public bool Write;
        public ShaderPassCompareFunc Compare;
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassStencilOp : int
    {
        Keep = 0,
        Zero = 1,
        Replace = 2,
        IncrSat = 3,
        DecrSat = 4,
        Invert = 5,
        Incr = 6,
        Decr = 7,
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassStencilAction
    {
        public ShaderPassCompareFunc Compare;
        public ShaderPassStencilOp PassOp;
        public ShaderPassStencilOp FailOp;
        public ShaderPassStencilOp DepthFailOp;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassStencilState
    {
        public bool Enable;
        public byte ReadMask;
        public byte WriteMask;
        public ShaderPassStencilAction FrontFace;
        public ShaderPassStencilAction BackFace;
    }

    internal class ShaderPass
    {
        public string Name = string.Empty;

        public byte[] VertexShader = [];
        public byte[] PixelShader = [];

        public ShaderPassConstantBuffer[] ConstantBuffers = [];
        public ShaderPassSampler[] Samplers = [];
        public ShaderPassMaterialProperty[] MaterialProperties = [];
        public ShaderPassTextureProperty[] TextureProperties = [];

        public ShaderPassCullMode Cull;
        public ShaderPassBlendState[] Blends = [];
        public ShaderPassDepthState DepthState;
        public ShaderPassStencilState StencilState;

        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct Native
        {
            public nint Name;

            public byte* VertexShader;
            public uint VertexShaderSize;
            public byte* PixelShader;
            public uint PixelShaderSize;

            public ShaderPassConstantBuffer.Native* ConstantBuffers;
            public uint ConstantBuffersSize;
            public ShaderPassSampler.Native* Samplers;
            public uint SamplersSize;
            public ShaderPassMaterialProperty.Native* MaterialProperties;
            public uint MaterialPropertiesSize;
            public ShaderPassTextureProperty.Native* TextureProperties;
            public uint TexturePropertiesSize;

            public ShaderPassCullMode Cull;
            public ShaderPassBlendState* Blends;
            public uint BlendsSize;
            public ShaderPassDepthState DepthState;
            public ShaderPassStencilState StencilState;
        }

        internal static unsafe ShaderPass GetAndFree(ref Native native)
        {
            var pass = new ShaderPass
            {
                Name = NativeString.GetAndFree(native.Name),
                Cull = native.Cull,
                DepthState = native.DepthState,
                StencilState = native.StencilState,
            };

            pass.VertexShader = new byte[native.VertexShaderSize];
            Marshal.Copy((nint)native.VertexShader, pass.VertexShader, 0, pass.VertexShader.Length);

            pass.PixelShader = new byte[native.PixelShaderSize];
            Marshal.Copy((nint)native.PixelShader, pass.PixelShader, 0, pass.PixelShader.Length);

            pass.ConstantBuffers = new ShaderPassConstantBuffer[native.ConstantBuffersSize];
            for (int i = 0; i < native.ConstantBuffersSize; i++)
            {
                pass.ConstantBuffers[i] = ShaderPassConstantBuffer.GetAndFree(ref native.ConstantBuffers[i]);
            }

            pass.Samplers = new ShaderPassSampler[native.SamplersSize];
            for (int i = 0; i < native.SamplersSize; i++)
            {
                pass.Samplers[i] = ShaderPassSampler.GetAndFree(ref native.Samplers[i]);
            }

            pass.MaterialProperties = new ShaderPassMaterialProperty[native.MaterialPropertiesSize];
            for (int i = 0; i < native.MaterialPropertiesSize; i++)
            {
                pass.MaterialProperties[i] = ShaderPassMaterialProperty.GetAndFree(ref native.MaterialProperties[i]);
            }

            pass.TextureProperties = new ShaderPassTextureProperty[native.TexturePropertiesSize];
            for (int i = 0; i < native.TexturePropertiesSize; i++)
            {
                pass.TextureProperties[i] = ShaderPassTextureProperty.GetAndFree(ref native.TextureProperties[i]);
            }

            pass.Blends = new ShaderPassBlendState[native.BlendsSize];
            for (int i = 0; i < native.BlendsSize; i++)
            {
                pass.Blends[i] = native.Blends[i];
            }

            return pass;
        }

        internal static unsafe void ToNative(ShaderPass value, out Native native)
        {
            native.Name = NativeString.New(value.Name);

            native.VertexShader = (byte*)Marshal.AllocHGlobal(value.VertexShader.Length);
            Marshal.Copy(value.VertexShader, 0, (nint)native.VertexShader, value.VertexShader.Length);
            native.VertexShaderSize = (uint)value.VertexShader.Length;

            native.PixelShader = (byte*)Marshal.AllocHGlobal(value.PixelShader.Length);
            Marshal.Copy(value.PixelShader, 0, (nint)native.PixelShader, value.PixelShader.Length);
            native.PixelShaderSize = (uint)value.PixelShader.Length;

            native.ConstantBuffers = (ShaderPassConstantBuffer.Native*)Marshal.AllocHGlobal(
                sizeof(ShaderPassConstantBuffer.Native) * value.ConstantBuffers.Length);
            native.ConstantBuffersSize = (uint)value.ConstantBuffers.Length;
            for (int i = 0; i < value.ConstantBuffers.Length; i++)
            {
                ShaderPassConstantBuffer.ToNative(value.ConstantBuffers[i], out native.ConstantBuffers[i]);
            }

            native.Samplers = (ShaderPassSampler.Native*)Marshal.AllocHGlobal(
                sizeof(ShaderPassSampler.Native) * value.Samplers.Length);
            native.SamplersSize = (uint)value.Samplers.Length;
            for (int i = 0; i < value.Samplers.Length; i++)
            {
                ShaderPassSampler.ToNative(value.Samplers[i], out native.Samplers[i]);
            }

            native.MaterialProperties = (ShaderPassMaterialProperty.Native*)Marshal.AllocHGlobal(
                sizeof(ShaderPassMaterialProperty.Native) * value.MaterialProperties.Length);
            native.MaterialPropertiesSize = (uint)value.MaterialProperties.Length;
            for (int i = 0; i < value.MaterialProperties.Length; i++)
            {
                ShaderPassMaterialProperty.ToNative(value.MaterialProperties[i], out native.MaterialProperties[i]);
            }

            native.TextureProperties = (ShaderPassTextureProperty.Native*)Marshal.AllocHGlobal(
                sizeof(ShaderPassTextureProperty.Native) * value.TextureProperties.Length);
            native.TexturePropertiesSize = (uint)value.TextureProperties.Length;
            for (int i = 0; i < value.TextureProperties.Length; i++)
            {
                ShaderPassTextureProperty.ToNative(value.TextureProperties[i], out native.TextureProperties[i]);
            }

            native.Cull = value.Cull;

            native.Blends = (ShaderPassBlendState*)Marshal.AllocHGlobal(
                sizeof(ShaderPassBlendState) * value.Blends.Length);
            native.BlendsSize = (uint)value.Blends.Length;
            for (int i = 0; i < value.Blends.Length; i++)
            {
                native.Blends[i] = value.Blends[i];
            }

            native.DepthState = value.DepthState;
            native.StencilState = value.StencilState;
        }

        internal static unsafe void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);

            Marshal.FreeHGlobal((nint)native.VertexShader);
            Marshal.FreeHGlobal((nint)native.PixelShader);

            for (int i = 0; i < native.ConstantBuffersSize; i++)
            {
                ShaderPassConstantBuffer.FreeNative(ref native.ConstantBuffers[i]);
            }
            Marshal.FreeHGlobal((nint)native.ConstantBuffers);

            for (int i = 0; i < native.SamplersSize; i++)
            {
                ShaderPassSampler.FreeNative(ref native.Samplers[i]);
            }
            Marshal.FreeHGlobal((nint)native.Samplers);

            for (int i = 0; i < native.MaterialPropertiesSize; i++)
            {
                ShaderPassMaterialProperty.FreeNative(ref native.MaterialProperties[i]);
            }
            Marshal.FreeHGlobal((nint)native.MaterialProperties);

            for (int i = 0; i < native.TexturePropertiesSize; i++)
            {
                ShaderPassTextureProperty.FreeNative(ref native.TextureProperties[i]);
            }
            Marshal.FreeHGlobal((nint)native.TextureProperties);

            Marshal.FreeHGlobal((nint)native.Blends);
        }
    }

    public unsafe partial class Shader : EngineObject
    {
        private nint m_Shader;

        [JsonProperty]
        internal string Name { get; set; } = string.Empty;

        [JsonProperty]
        internal ShaderProperty[] Properties { get; set; } = [];

        [JsonProperty]
        internal ShaderPass[] Passes
        {
            get
            {
                int count = Shader_GetPassCount(m_Shader);
                ShaderPass.Native* pPasses = (ShaderPass.Native*)Marshal.AllocHGlobal(sizeof(ShaderPass.Native) * count);
                Shader_GetPasses(m_Shader, pPasses);

                var passes = new ShaderPass[count];
                for (int i = 0; i < count; i++)
                {
                    passes[i] = ShaderPass.GetAndFree(ref pPasses[i]);
                }

                Marshal.FreeHGlobal((nint)pPasses);
                return passes;
            }

            set
            {
                ShaderPass.Native* pPasses = (ShaderPass.Native*)Marshal.AllocHGlobal(
                    sizeof(ShaderPass.Native) * value.Length);

                for (int i = 0; i < value.Length; i++)
                {
                    ShaderPass.ToNative(value[i], out pPasses[i]);
                }

                Shader_SetPasses(m_Shader, pPasses, value.Length);

                for (int i = 0; i < value.Length; i++)
                {
                    ShaderPass.FreeNative(ref pPasses[i]);
                }

                Marshal.FreeHGlobal((nint)pPasses);
            }
        }

        [NativeFunction]
        private static partial nint Shader_New();

        [NativeFunction]
        private static partial void Shader_Delete(nint pShader);

        [NativeFunction]
        private static partial int Shader_GetPassCount(nint pShader);

        [NativeFunction]
        private static partial void Shader_GetPasses(nint pShader, ShaderPass.Native* pPasses);

        [NativeFunction]
        private static partial void Shader_SetPasses(nint pShader, ShaderPass.Native* pPasses, int passCount);
    }

    internal sealed class ShaderPassColorWriteMaskJsonConverter : JsonConverter<ShaderPassColorWriteMask>
    {
        public override ShaderPassColorWriteMask ReadJson(JsonReader reader, Type objectType, ShaderPassColorWriteMask existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var mask = ShaderPassColorWriteMask.None;
            string? value = reader.ReadAsString();

            if (!string.IsNullOrEmpty(value))
            {
                if (value.Contains('r', StringComparison.OrdinalIgnoreCase))
                {
                    mask |= ShaderPassColorWriteMask.Red;
                }

                if (value.Contains('g', StringComparison.OrdinalIgnoreCase))
                {
                    mask |= ShaderPassColorWriteMask.Green;
                }

                if (value.Contains('b', StringComparison.OrdinalIgnoreCase))
                {
                    mask |= ShaderPassColorWriteMask.Blue;
                }

                if (value.Contains('a', StringComparison.OrdinalIgnoreCase))
                {
                    mask |= ShaderPassColorWriteMask.Alpha;
                }
            }

            return mask;
        }

        public override void WriteJson(JsonWriter writer, ShaderPassColorWriteMask value, JsonSerializer serializer)
        {
            Span<char> buffer = stackalloc char[4];
            int len = 0;

            if ((value & ShaderPassColorWriteMask.Red) == ShaderPassColorWriteMask.Red)
            {
                buffer[len++] = 'r';
            }

            if ((value & ShaderPassColorWriteMask.Green) == ShaderPassColorWriteMask.Green)
            {
                buffer[len++] = 'g';
            }

            if ((value & ShaderPassColorWriteMask.Blue) == ShaderPassColorWriteMask.Blue)
            {
                buffer[len++] = 'b';
            }

            if ((value & ShaderPassColorWriteMask.Alpha) == ShaderPassColorWriteMask.Alpha)
            {
                buffer[len++] = 'a';
            }

            if (len == 0)
            {
                writer.WriteNull();
            }
            else
            {
                writer.WriteValue(new string(buffer[..len]));
            }
        }
    }

    internal sealed class ShaderJsonNamingStrategy : NamingStrategy
    {
        protected override string ResolvePropertyName(string name)
        {
            if (name.Length == 0)
            {
                return name;
            }

            return char.ToLower(name[0]) + name[1..];
        }
    }
}
