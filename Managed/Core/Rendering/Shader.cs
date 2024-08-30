using DX12Demo.Core.Binding;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using Newtonsoft.Json.Serialization;
using System.Numerics;
using System.Runtime.InteropServices;

namespace DX12Demo.Core.Rendering
{
    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPropertyType
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderDefaultTexture
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

    internal class ShaderPassMaterialProperty
    {
        public string Name = string.Empty;
        public ShaderPropertyType Type;
        public int Offset;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public ShaderPropertyType Type;
            public int Offset;
        }

        internal static ShaderPassMaterialProperty FromNative(in Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            Type = native.Type,
            Offset = native.Offset
        };
    }

    internal class ShaderPassTextureProperty
    {
        public string Name = string.Empty;
        public int ShaderRegisterTexture;
        public int ShaderRegisterSampler;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public int ShaderRegisterTexture;
            public int ShaderRegisterSampler;
        }

        internal static ShaderPassTextureProperty FromNative(in Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegisterTexture = native.ShaderRegisterTexture,
            ShaderRegisterSampler = native.ShaderRegisterSampler
        };
    }

    internal class ShaderPassConstantBuffer
    {
        public string Name = string.Empty;
        public int ShaderRegister;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public int ShaderRegister;
        }

        internal static ShaderPassConstantBuffer FromNative(in Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegister = native.ShaderRegister
        };
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassCullMode
    {
        Off = 0,
        Front = 1,
        Back = 2,
    }

    [JsonConverter(typeof(StringEnumConverter), typeof(ShaderJsonNamingStrategy))]
    internal enum ShaderPassBlend
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
    internal enum ShaderPassBlendOp
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    }

    [Flags]
    [JsonConverter(typeof(ShaderPassColorWriteMaskJsonConverter))]
    internal enum ShaderPassColorWriteMask
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
    internal enum ShaderPassCompareFunc
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
    internal enum ShaderPassStencilOp
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

    public class Shader : EngineObject
    {
        [JsonProperty]
        internal string Name { get; set; } = string.Empty;

        [JsonProperty]
        internal ShaderProperty[] Properties { get; set; } = [];

        internal int GetShaderPassCount()
        {
            throw new NotImplementedException();
        }

        internal string GetShaderPassName(int index)
        {
            throw new NotImplementedException();
        }

        internal byte[] GetShaderPassVertexShader(int index)
        {
            throw new NotImplementedException();
        }

        internal byte[] GetShaderPassPixelShader(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassConstantBuffer[] GetShaderPassConstantBuffers(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassMaterialProperty[] GetShaderPassMaterialProperties(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassTextureProperty[] GetShaderPassTextureProperties(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassCullMode GetShaderPassCullMode(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassBlendState[] GetShaderPassBlendStates(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassDepthState GetShaderPassDepthState(int index)
        {
            throw new NotImplementedException();
        }

        internal ShaderPassStencilState GetShaderPassStencilState(int index)
        {
            throw new NotImplementedException();
        }
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

    internal sealed class ShaderJsonConverter : JsonConverter<Shader>
    {
        public override Shader? ReadJson(JsonReader reader, Type objectType, Shader? existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            throw new NotImplementedException();
        }

        public override void WriteJson(JsonWriter writer, Shader? value, JsonSerializer serializer)
        {
            if (value == null)
            {
                writer.WriteNull();
                return;
            }

            writer.WriteStartObject();

            writer.WritePropertyName(nameof(Shader.Name));
            writer.WriteValue(value.Name);

            writer.WritePropertyName(nameof(Shader.Properties));
            serializer.Serialize(writer, value.Properties);

            writer.WritePropertyName("Passes");
            writer.WriteStartArray();
            for (int i = 0; i < value.GetShaderPassCount(); i++)
            {
                writer.WriteStartObject();

                writer.WritePropertyName("Name");
                writer.WriteValue(value.GetShaderPassName(i));

                writer.WritePropertyName("VertexShader");
                writer.WriteValue(value.GetShaderPassVertexShader(i));

                writer.WritePropertyName("PixelShader");
                writer.WriteValue(value.GetShaderPassPixelShader(i));

                // TODO ...

                writer.WriteEndObject();
            }
            writer.WriteEndArray();

            writer.WriteEndObject();
        }
    }
}
