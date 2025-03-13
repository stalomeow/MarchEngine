using March.Core.Interop;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    internal class ShaderTexture : INativeMarshal<ShaderTexture, ShaderTexture.Native>
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
            public NativeBool HasSampler;
            public uint ShaderRegisterSampler;
            public uint RegisterSpaceSampler;
        }

        public static ShaderTexture FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegisterTexture = native.ShaderRegisterTexture,
            RegisterSpaceTexture = native.RegisterSpaceTexture,
            HasSampler = native.HasSampler,
            ShaderRegisterSampler = native.ShaderRegisterSampler,
            RegisterSpaceSampler = native.RegisterSpaceSampler,
        };

        public static void ToNative(ShaderTexture value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegisterTexture = value.ShaderRegisterTexture;
            native.RegisterSpaceTexture = value.RegisterSpaceTexture;
            native.HasSampler = value.HasSampler;
            native.ShaderRegisterSampler = value.ShaderRegisterSampler;
            native.RegisterSpaceSampler = value.RegisterSpaceSampler;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal class ShaderStaticSampler : INativeMarshal<ShaderStaticSampler, ShaderStaticSampler.Native>
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

        public static ShaderStaticSampler FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
        };

        public static void ToNative(ShaderStaticSampler value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal class ShaderBuffer : INativeMarshal<ShaderBuffer, ShaderBuffer.Native>
    {
        public string Name = string.Empty;
        public uint ShaderRegister;
        public uint RegisterSpace;
        public bool IsConstantBuffer;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint ShaderRegister;
            public uint RegisterSpace;
            public NativeBool IsConstantBuffer;
        }

        public static ShaderBuffer FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
            IsConstantBuffer = native.IsConstantBuffer,
        };

        public static void ToNative(ShaderBuffer value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
            native.IsConstantBuffer = value.IsConstantBuffer;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal enum ShaderProgramType
    {
        Vertex,
        Pixel,
        Domain,
        Hull,
        Geometry,
    };

    internal class ShaderProgram : INativeMarshal<ShaderProgram, ShaderProgram.Native>
    {
        public int Type;
        public string[] Keywords = [];
        public byte[] Hash = [];
        public byte[] Binary = [];
        public ShaderBuffer[] SrvCbvBuffers = [];
        public ShaderTexture[] SrvTextures = [];
        public ShaderBuffer[] UavBuffers = [];
        public ShaderTexture[] UavTextures = [];
        public ShaderStaticSampler[] StaticSamplers = [];
        public uint ThreadGroupSizeX;
        public uint ThreadGroupSizeY;
        public uint ThreadGroupSizeZ;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public int Type;
            public NativeArray<nint> Keywords;
            public NativeArray<byte> Hash;
            public NativeArray<byte> Binary;
            public NativeArrayMarshal<ShaderBuffer> SrvCbvBuffers;
            public NativeArrayMarshal<ShaderTexture> SrvTextures;
            public NativeArrayMarshal<ShaderBuffer> UavBuffers;
            public NativeArrayMarshal<ShaderTexture> UavTextures;
            public NativeArrayMarshal<ShaderStaticSampler> StaticSamplers;
            public uint ThreadGroupSizeX;
            public uint ThreadGroupSizeY;
            public uint ThreadGroupSizeZ;
        }

        public static ShaderProgram FromNative(ref Native native) => new()
        {
            Type = native.Type,
            Keywords = native.Keywords.ConvertValue((in nint s) => NativeString.Get(s)),
            Hash = native.Hash.Value,
            Binary = native.Binary.Value,
            SrvCbvBuffers = native.SrvCbvBuffers.Value,
            SrvTextures = native.SrvTextures.Value,
            UavBuffers = native.UavBuffers.Value,
            UavTextures = native.UavTextures.Value,
            StaticSamplers = native.StaticSamplers.Value,
            ThreadGroupSizeX = native.ThreadGroupSizeX,
            ThreadGroupSizeY = native.ThreadGroupSizeY,
            ThreadGroupSizeZ = native.ThreadGroupSizeZ,
        };

        public static void ToNative(ShaderProgram value, out Native native)
        {
            native.Type = value.Type;
            native.Keywords = new NativeArray<nint>(value.Keywords.Length);
            native.Hash = value.Hash;
            native.Binary = value.Binary;
            native.SrvCbvBuffers = value.SrvCbvBuffers;
            native.SrvTextures = value.SrvTextures;
            native.UavBuffers = value.UavBuffers;
            native.UavTextures = value.UavTextures;
            native.StaticSamplers = value.StaticSamplers;
            native.ThreadGroupSizeX = value.ThreadGroupSizeX;
            native.ThreadGroupSizeY = value.ThreadGroupSizeY;
            native.ThreadGroupSizeZ = value.ThreadGroupSizeZ;

            for (int i = 0; i < value.Keywords.Length; i++)
            {
                native.Keywords[i] = NativeString.New(value.Keywords[i]);
            }
        }

        public static void FreeNative(ref Native native)
        {
            for (int i = 0; i < native.Keywords.Length; i++)
            {
                NativeString.Free(native.Keywords[i]);
            }

            native.Hash.Dispose();
            native.Keywords.Dispose();
            native.Binary.Dispose();
            native.SrvCbvBuffers.Dispose();
            native.SrvTextures.Dispose();
            native.UavBuffers.Dispose();
            native.UavTextures.Dispose();
            native.StaticSamplers.Dispose();
        }
    }

    internal enum ShaderPropertyType
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    }

    internal class ShaderPropertyAttribute
    {
        public string Name = string.Empty;
        public string? Arguments = null;
    }

    internal class ShaderProperty
    {
        public string Name = string.Empty;
        public string Label = string.Empty;
        public string Tooltip = string.Empty;
        public List<ShaderPropertyAttribute> Attributes = [];
        public ShaderPropertyType Type;

        public float DefaultFloat;
        public int DefaultInt;
        public Color DefaultColor;
        public Vector4 DefaultVector;

        public TextureDimension TexDimension;
        public DefaultTexture DefaultTex;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public ShaderPropertyType Type;

            public float DefaultFloat;
            public int DefaultInt;
            public Color DefaultColor;
            public Vector4 DefaultVector;

            public TextureDimension TexDimension;
            public DefaultTexture DefaultTex;
        }

        public static void ToNative(ShaderProperty value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Type = value.Type;
            native.DefaultFloat = value.DefaultFloat;
            native.DefaultInt = value.DefaultInt;
            native.DefaultColor = value.DefaultColor;
            native.DefaultVector = value.DefaultVector;
            native.TexDimension = value.TexDimension;
            native.DefaultTex = value.DefaultTex;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    internal class ShaderPropertyLocation : INativeMarshal<ShaderPropertyLocation, ShaderPropertyLocation.Native>
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

        public static ShaderPropertyLocation FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            Offset = native.Offset,
            Size = native.Size,
        };

        public static void ToNative(ShaderPropertyLocation value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Offset = value.Offset;
            native.Size = value.Size;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    [JsonConverter(typeof(OptionalShaderPropertyIdJsonConverter))]
    internal struct OptionalShaderPropertyId(string propertyName)
    {
        public NativeBool HasValue = true;
        public int Value = ShaderUtility.GetIdFromString(propertyName);
    }

    internal sealed class OptionalShaderPropertyIdJsonConverter : JsonConverter<OptionalShaderPropertyId>
    {
        public override OptionalShaderPropertyId ReadJson(JsonReader reader, Type objectType, OptionalShaderPropertyId existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            if (reader.TokenType == JsonToken.Null)
            {
                return default;
            }

            string propertyName = reader.Value!.ToString()!;
            return new OptionalShaderPropertyId(propertyName);
        }

        public override void WriteJson(JsonWriter writer, OptionalShaderPropertyId value, JsonSerializer serializer)
        {
            if (value.HasValue)
            {
                writer.WriteValue(ShaderUtility.GetStringFromId(value.Value));
            }
            else
            {
                writer.WriteNull();
            }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassVar<T> : IEquatable<ShaderPassVar<T>> where T : unmanaged
    {
        [JsonProperty("PropertyName")]
        public OptionalShaderPropertyId PropertyId;
        public T Value;

        public ShaderPassVar(string propertyName)
        {
            PropertyId = new OptionalShaderPropertyId(propertyName);
            Value = default;
        }

        public ShaderPassVar(T value)
        {
            PropertyId = default;
            Value = value;
        }

        public readonly bool Equals(ShaderPassVar<T> other)
        {
            // 两个都使用 Value -> 比较 Value
            // 两个都使用 PropertyId -> 比较 PropertyId
            // 一个使用 Value，一个使用 PropertyId -> false

            if (!PropertyId.HasValue && !other.PropertyId.HasValue)
            {
                return EqualityComparer<T>.Default.Equals(Value, other.Value);
            }

            return PropertyId.HasValue && other.PropertyId.HasValue && PropertyId.Value == other.PropertyId.Value;
        }

        public override readonly bool Equals([NotNullWhen(true)] object? obj)
        {
            return (obj is ShaderPassVar<T> v) && Equals(v);
        }

        public override readonly int GetHashCode()
        {
            return PropertyId.HasValue ? HashCode.Combine(true, PropertyId.Value, default(T)) : HashCode.Combine(false, default(int), Value);
        }

        public static bool operator ==(ShaderPassVar<T> left, ShaderPassVar<T> right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(ShaderPassVar<T> left, ShaderPassVar<T> right)
        {
            return !left.Equals(right);
        }

        public static implicit operator ShaderPassVar<T>(T value) => new(value);
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassBlendFormula : IEquatable<ShaderPassBlendFormula>
    {
        public ShaderPassVar<BlendMode> Src;
        public ShaderPassVar<BlendMode> Dest;
        public ShaderPassVar<BlendOp> Op;

        public readonly override bool Equals(object? obj)
        {
            return obj is ShaderPassBlendFormula formula && Equals(formula);
        }

        public readonly bool Equals(ShaderPassBlendFormula other)
        {
            return Src == other.Src &&
                   Dest == other.Dest &&
                   Op == other.Op;
        }

        public readonly override int GetHashCode()
        {
            return HashCode.Combine(Src, Dest, Op);
        }

        public static readonly ShaderPassBlendFormula Default = new()
        {
            Src = BlendMode.One,
            Dest = BlendMode.Zero,
            Op = BlendOp.Add
        };
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassBlendState : IEquatable<ShaderPassBlendState>
    {
        public NativeBool Enable;
        public ShaderPassVar<ColorWriteMask> WriteMask;
        public ShaderPassBlendFormula Rgb;
        public ShaderPassBlendFormula Alpha;

        public readonly override bool Equals(object? obj)
        {
            return obj is ShaderPassBlendState state && Equals(state);
        }

        public readonly bool Equals(ShaderPassBlendState other)
        {
            return Enable == other.Enable &&
                   WriteMask == other.WriteMask &&
                   Rgb.Equals(other.Rgb) &&
                   Alpha.Equals(other.Alpha);
        }

        public readonly override int GetHashCode()
        {
            return HashCode.Combine(Enable, WriteMask, Rgb, Alpha);
        }

        public static readonly ShaderPassBlendState Default = new()
        {
            Enable = false,
            WriteMask = ColorWriteMask.All,
            Rgb = ShaderPassBlendFormula.Default,
            Alpha = ShaderPassBlendFormula.Default
        };
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassDepthState
    {
        public NativeBool Enable;
        public ShaderPassVar<NativeBool> Write;
        public ShaderPassVar<CompareFunction> Compare;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassStencilAction
    {
        public ShaderPassVar<CompareFunction> Compare;
        public ShaderPassVar<StencilOp> PassOp;
        public ShaderPassVar<StencilOp> FailOp;
        public ShaderPassVar<StencilOp> DepthFailOp;
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassStencilState
    {
        public NativeBool Enable;
        public ShaderPassVar<byte> Ref;
        public ShaderPassVar<byte> ReadMask;
        public ShaderPassVar<byte> WriteMask;
        public ShaderPassStencilAction FrontFace;
        public ShaderPassStencilAction BackFace;

        public static readonly ShaderPassStencilState Default = new()
        {
            Ref = 0,
            Enable = false,
            ReadMask = 0xFF,
            WriteMask = 0xFF,
            FrontFace = new()
            {
                Compare = CompareFunction.Always,
                PassOp = StencilOp.Keep,
                FailOp = StencilOp.Keep,
                DepthFailOp = StencilOp.Keep,
            },
            BackFace = new()
            {
                Compare = CompareFunction.Always,
                PassOp = StencilOp.Keep,
                FailOp = StencilOp.Keep,
                DepthFailOp = StencilOp.Keep,
            },
        };
    }

    internal class ShaderPassTag(string key, string value) : INativeMarshal<ShaderPassTag, ShaderPassTag.Native>
    {
        public string Key = key;
        public string Value = value;

        public ShaderPassTag() : this(string.Empty, string.Empty) { }

        [StructLayout(LayoutKind.Sequential)]
        internal unsafe struct Native
        {
            public nint Key;
            public nint Value;
        }

        public static ShaderPassTag FromNative(ref Native native) => new()
        {
            Key = NativeString.Get(native.Key),
            Value = NativeString.Get(native.Value),
        };

        public static void ToNative(ShaderPassTag value, out Native native)
        {
            native.Key = NativeString.New(value.Key);
            native.Value = NativeString.New(value.Value);
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Key);
            NativeString.Free(native.Value);
        }
    }

    internal class ShaderPass : INativeMarshal<ShaderPass, ShaderPass.Native>
    {
        public string Name = string.Empty;
        public ShaderPassTag[] Tags = [];
        public ShaderProgram[] Programs = [];

        public ShaderPassVar<CullMode> Cull;
        public ShaderPassBlendState[] Blends = []; // 如果长度大于 1 则使用 Independent Blend
        public ShaderPassDepthState DepthState;
        public ShaderPassStencilState StencilState;

        [StructLayout(LayoutKind.Sequential)]
        internal unsafe struct Native
        {
            public nint Name;
            public NativeArrayMarshal<ShaderPassTag> Tags;
            public NativeArrayMarshal<ShaderProgram> Programs;
            public ShaderPassVar<CullMode> Cull;
            public NativeArray<ShaderPassBlendState> Blends;
            public ShaderPassDepthState DepthState;
            public ShaderPassStencilState StencilState;
        }

        public static unsafe ShaderPass FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            Tags = native.Tags.Value,
            Programs = native.Programs.Value,
            Cull = native.Cull,
            Blends = native.Blends.Value,
            DepthState = native.DepthState,
            StencilState = native.StencilState,
        };

        public static unsafe void ToNative(ShaderPass value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Tags = value.Tags;
            native.Programs = value.Programs;
            native.Cull = value.Cull;
            native.Blends = value.Blends;
            native.DepthState = value.DepthState;
            native.StencilState = value.StencilState;
        }

        public static unsafe void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
            native.Tags.Dispose();
            native.Programs.Dispose();
            native.Blends.Dispose();
        }
    }

    [NativeTypeName("ShaderUtils")]
    public static partial class ShaderUtility
    {
        [NativeMethod]
        public static partial int GetIdFromString(StringLike name);

        [NativeMethod]
        public static partial string GetStringFromId(int id);
    }

    public unsafe partial class Shader : NativeMarchObject
    {
        private ShaderProperty[] m_Properties = [];

        [JsonProperty]
        [NativeProperty]
        public partial string Name { get; set; }

        [JsonProperty]
        internal ShaderProperty[] Properties
        {
            get => m_Properties;

            set
            {
                m_Properties = value;
                ClearProperties();

                foreach (ShaderProperty prop in value)
                {
                    ShaderProperty.ToNative(prop, out ShaderProperty.Native native);

                    try
                    {
                        SetProperty(&native);
                    }
                    finally
                    {
                        ShaderProperty.FreeNative(ref native);
                    }
                }
            }
        }

        [JsonProperty]
        internal ShaderPass[] Passes
        {
            get
            {
                GetPasses(out nint passes);
                return NativeArrayMarshal<ShaderPass>.GetAndFree(passes);
            }

            set
            {
                using NativeArrayMarshal<ShaderPass> passes = value;
                SetPasses(passes);
            }
        }

        [JsonProperty]
        [NativeProperty]
        internal partial uint MaterialConstantBufferSize { get; set; }

        [JsonProperty]
        internal ShaderPropertyLocation[] PropertyLocations
        {
            get
            {
                GetPropertyLocations(out nint locations);
                return NativeArrayMarshal<ShaderPropertyLocation>.GetAndFree(locations);
            }

            set
            {
                using NativeArrayMarshal<ShaderPropertyLocation> locations = value;
                SetPropertyLocations(locations);
            }
        }

        public Shader() : base(New()) { }

        internal bool CompilePass(int passIndex, string filename, string source, List<string> outWarnings, List<string> outErrors)
        {
            nint nativeWarnings = nint.Zero;
            nint nativeError = nint.Zero;
            bool success = CompilePass(passIndex, filename, source, &nativeWarnings, &nativeError);

            if (nativeWarnings != nint.Zero)
            {
                using var warnings = (NativeArray<nint>)nativeWarnings;

                for (int i = 0; i < warnings.Length; i++)
                {
                    outWarnings.Add(NativeString.GetAndFree(warnings[i]));
                }
            }

            if (nativeError != nint.Zero)
            {
                outErrors.Add(NativeString.GetAndFree(nativeError));
            }

            return success;
        }

        [NativeProperty]
        internal static partial int MaterialConstantBufferId { get; }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void ClearProperties();

        [NativeMethod]
        private partial void SetProperty(ShaderProperty.Native* prop);

        [NativeMethod]
        private partial void GetPasses(out nint passes);

        [NativeMethod]
        private partial void SetPasses(NativeArrayMarshal<ShaderPass> passes);

        [NativeMethod]
        private partial void GetPropertyLocations(out nint locations);

        [NativeMethod]
        private partial void SetPropertyLocations(NativeArrayMarshal<ShaderPropertyLocation> locations);

        [NativeMethod]
        private partial bool CompilePass(int passIndex, string filename, string source, nint* warnings, nint* error);
    }

    internal class ComputeShaderKernel : INativeMarshal<ComputeShaderKernel, ComputeShaderKernel.Native>
    {
        public string Name = string.Empty;
        public ShaderProgram[] Programs = [];

        [StructLayout(LayoutKind.Sequential)]
        internal unsafe struct Native
        {
            public nint Name;
            public NativeArrayMarshal<ShaderProgram> Programs;
        }

        public static unsafe ComputeShaderKernel FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            Programs = native.Programs.Value,
        };

        public static unsafe void ToNative(ComputeShaderKernel value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Programs = value.Programs;
        }

        public static unsafe void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
            native.Programs.Dispose();
        }
    }

    public unsafe partial class ComputeShader : NativeMarchObject
    {
        [JsonProperty]
        [NativeProperty]
        public partial string Name { get; set; }

        [JsonProperty]
        internal ComputeShaderKernel[] Kernels
        {
            get
            {
                GetKernels(out nint kernels);
                return NativeArrayMarshal<ComputeShaderKernel>.GetAndFree(kernels);
            }

            set
            {
                using NativeArrayMarshal<ComputeShaderKernel> kernels = value;
                SetKernels(kernels);
            }
        }

        public ComputeShader() : base(New()) { }

        internal bool Compile(string filename, string source, List<string> outWarnings, List<string> outErrors)
        {
            nint nativeWarnings = nint.Zero;
            nint nativeError = nint.Zero;
            bool success = Compile(filename, source, &nativeWarnings, &nativeError);

            if (nativeWarnings != nint.Zero)
            {
                using var warnings = (NativeArray<nint>)nativeWarnings;

                for (int i = 0; i < warnings.Length; i++)
                {
                    outWarnings.Add(NativeString.GetAndFree(warnings[i]));
                }
            }

            if (nativeError != nint.Zero)
            {
                outErrors.Add(NativeString.GetAndFree(nativeError));
            }

            return success;
        }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void GetKernels(out nint passes);

        [NativeMethod]
        private partial void SetKernels(NativeArrayMarshal<ComputeShaderKernel> passes);

        [NativeMethod]
        private partial bool Compile(string filename, string source, nint* warnings, nint* error);
    }
}
