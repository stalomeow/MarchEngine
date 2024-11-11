using March.Core.Diagnostics;
using March.Core.Interop;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Collections.Immutable;
using System.Diagnostics.CodeAnalysis;
using System.Numerics;
using System.Runtime.InteropServices;

namespace March.Core.Rendering
{
    internal class ShaderConstantBuffer : INativeMarshal<ShaderConstantBuffer, ShaderConstantBuffer.Native>
    {
        public string Name = string.Empty;
        public uint ShaderRegister;
        public uint RegisterSpace;
        public uint UnalignedSize;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint ShaderRegister;
            public uint RegisterSpace;
            public uint UnalignedSize;
        }

        public static ShaderConstantBuffer FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
            UnalignedSize = native.UnalignedSize,
        };

        public static void ToNative(ShaderConstantBuffer value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
            native.UnalignedSize = value.UnalignedSize;
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
            public bool HasSampler;
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

    internal enum ShaderProgramType
    {
        Vertex,
        Pixel,
        NumTypes,
    };

    internal class ShaderProgram : INativeMarshal<ShaderProgram, ShaderProgram.Native>
    {
        public ShaderProgramType Type;
        public string[] Keywords = [];
        public byte[] Hash = [];
        public byte[] Binary = [];
        public ShaderConstantBuffer[] ConstantBuffers = [];
        public ShaderStaticSampler[] StaticSamplers = [];
        public ShaderTexture[] Textures = [];

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public ShaderProgramType Type;
            public NativeArray<nint> Keywords;
            public NativeArray<byte> Hash;
            public NativeArray<byte> Binary;
            public NativeArrayMarshal<ShaderConstantBuffer> ConstantBuffers;
            public NativeArrayMarshal<ShaderStaticSampler> StaticSamplers;
            public NativeArrayMarshal<ShaderTexture> Textures;
        }

        public static ShaderProgram FromNative(ref Native native) => new()
        {
            Type = native.Type,
            Keywords = native.Keywords.ConvertValue((in nint s) => NativeString.Get(s)),
            Hash = native.Hash.Value,
            Binary = native.Binary.Value,
            ConstantBuffers = native.ConstantBuffers.Value,
            StaticSamplers = native.StaticSamplers.Value,
            Textures = native.Textures.Value,
        };

        public static void ToNative(ShaderProgram value, out Native native)
        {
            native.Type = value.Type;
            native.Keywords = new NativeArray<nint>(value.Keywords.Length);
            native.Hash = value.Hash;
            native.Binary = value.Binary;
            native.ConstantBuffers = value.ConstantBuffers;
            native.StaticSamplers = value.StaticSamplers;
            native.Textures = value.Textures;

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
            native.ConstantBuffers.Dispose();
            native.StaticSamplers.Dispose();
            native.Textures.Dispose();
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

    internal enum ShaderDefaultTexture
    {
        Black = 0, // RGBA: 0, 0, 0, 1
        White = 1, // RGBA: 1, 1, 1, 1
        Bump = 2,  // RGBA: 0.5, 0.5, 1, 1
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
        public ShaderPropertyAttribute[] Attributes = [];
        public ShaderPropertyType Type;

        public float DefaultFloat;
        public int DefaultInt;
        public Color DefaultColor;
        public Vector4 DefaultVector;
        public ShaderDefaultTexture DefaultTexture;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public ShaderPropertyType Type;

            public float DefaultFloat;
            public int DefaultInt;
            public Color DefaultColor;
            public Vector4 DefaultVector;
            public ShaderDefaultTexture DefaultTexture;
        }

        public static void ToNative(ShaderProperty value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Type = value.Type;
            native.DefaultFloat = value.DefaultFloat;
            native.DefaultInt = value.DefaultInt;
            native.DefaultColor = value.DefaultColor;
            native.DefaultVector = value.DefaultVector;
            native.DefaultTexture = value.DefaultTexture;
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
        public bool HasValue = true;
        public int Value = Shader.GetNameId(propertyName);
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
                writer.WriteValue(Shader.GetIdName(value.Value));
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
        public bool Enable;
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
        public bool Enable;
        public ShaderPassVar<bool> Write;
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
        public bool Enable;
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
        public ShaderPropertyLocation[] PropertyLocations = [];
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
            public NativeArrayMarshal<ShaderPropertyLocation> PropertyLocations;
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
            PropertyLocations = native.PropertyLocations.Value,
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
            native.PropertyLocations = value.PropertyLocations;
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
            native.PropertyLocations.Dispose();
            native.Programs.Dispose();
            native.Blends.Dispose();
        }
    }

    public unsafe partial class Shader : NativeMarchObject
    {
        private ShaderProperty[] m_Properties = [];

        [JsonProperty]
        public string Name
        {
            get
            {
                nint name = Shader_GetName(NativePtr);
                return NativeString.GetAndFree(name);
            }

            internal set
            {
                using NativeString v = value;
                Shader_SetName(NativePtr, v.Data);
            }
        }

        [JsonProperty]
        public ImmutableArray<string> Warnings { get; private set; } = [];

        [JsonProperty]
        public ImmutableArray<string> Errors { get; private set; } = [];

        [JsonProperty]
        internal ShaderProperty[] Properties
        {
            get => m_Properties;

            set
            {
                m_Properties = value;
                Shader_ClearProperties(NativePtr);

                foreach (ShaderProperty prop in value)
                {
                    ShaderProperty.ToNative(prop, out ShaderProperty.Native native);

                    try
                    {
                        Shader_SetProperty(NativePtr, &native);
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
                nint passes = nint.Zero;
                Shader_GetPasses(NativePtr, &passes);
                return NativeArrayMarshal<ShaderPass>.GetAndFree(passes);
            }

            set
            {
                using NativeArrayMarshal<ShaderPass> passes = value;
                Shader_SetPasses(NativePtr, passes);
            }
        }

        public Shader() : base(Shader_New()) { }

        protected override void Dispose(bool disposing)
        {
            Shader_Delete(NativePtr);
        }

        public bool HasWarningOrError => !Warnings.IsEmpty || !Errors.IsEmpty;

        internal void ClearWarningsAndErrors()
        {
            Warnings = Warnings.Clear();
            Errors = Errors.Clear();
        }

        internal bool AddWarning(string warning)
        {
            if (!Warnings.Contains(warning))
            {
                Warnings = Warnings.Add(warning);
                return true;
            }

            return false;
        }

        internal bool AddError(string error)
        {
            if (!Errors.Contains(error))
            {
                Errors = Errors.Add(error);
                return true;
            }

            return false;
        }

        internal bool CompilePass(int passIndex, string filename, string source)
        {
            using NativeString f = filename;
            using NativeString s = source;

            nint nativeWarnings = nint.Zero;
            nint nativeError = nint.Zero;
            bool success = Shader_CompilePass(NativePtr, passIndex, f.Data, s.Data, &nativeWarnings, &nativeError);

            if (nativeWarnings != nint.Zero)
            {
                using var warnings = (NativeArray<nint>)nativeWarnings;

                for (int i = 0; i < warnings.Length; i++)
                {
                    string w = NativeString.GetAndFree(warnings[i]);

                    if (AddWarning(w))
                    {
                        Log.Message(LogLevel.Warning, w);
                    }
                }
            }

            if (nativeError != nint.Zero)
            {
                string e = NativeString.GetAndFree(nativeError);

                if (AddError(e))
                {
                    Log.Message(LogLevel.Error, e);
                }
            }

            return success;
        }

        #region EngineShaderPath

        private static string? s_CachedEngineShaderPath;

        /// <summary>
        /// 引擎内置 Shader 的路径 (Unix Style)
        /// </summary>
        public static string EngineShaderPath
        {
            get
            {
                if (s_CachedEngineShaderPath == null)
                {
                    nint s = Shader_GetEngineShaderPathUnixStyle();
                    s_CachedEngineShaderPath = NativeString.GetAndFree(s);
                }

                return s_CachedEngineShaderPath;
            }
        }

        #endregion

        #region Id Name Conversion

        public static int GetNameId(StringLike name)
        {
            using NativeString n = name;
            return Shader_GetNameId(n.Data);
        }

        public static string GetIdName(int id)
        {
            nint n = Shader_GetIdName(id);
            return NativeString.GetAndFree(n);
        }

        #endregion

        #region Native

        [NativeFunction]
        private static partial nint Shader_New();

        [NativeFunction]
        private static partial void Shader_Delete(nint pShader);

        [NativeFunction]
        private static partial nint Shader_GetName(nint pShader);

        [NativeFunction]
        private static partial void Shader_SetName(nint pShader, nint name);

        [NativeFunction]
        private static partial void Shader_ClearProperties(nint pShader);

        [NativeFunction]
        private static partial void Shader_SetProperty(nint pShader, ShaderProperty.Native* prop);

        [NativeFunction]
        private static partial void Shader_GetPasses(nint pShader, nint* passes);

        [NativeFunction]
        private static partial void Shader_SetPasses(nint pShader, NativeArrayMarshal<ShaderPass> passes);

        [NativeFunction]
        private static partial bool Shader_CompilePass(nint pShader, int passIndex, nint filename, nint source, nint* warnings, nint* error);

        [NativeFunction]
        private static partial nint Shader_GetEngineShaderPathUnixStyle();

        [NativeFunction]
        private static partial int Shader_GetNameId(nint name);

        [NativeFunction]
        private static partial nint Shader_GetIdName(int id);

        #endregion
    }
}
