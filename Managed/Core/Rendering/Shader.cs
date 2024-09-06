using DX12Demo.Core.Binding;
using Newtonsoft.Json;
using Newtonsoft.Json.Serialization;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace DX12Demo.Core.Rendering
{
    internal enum ShaderPropertyType : int
    {
        Float = 0,
        Int = 1,
        Color = 2,
        Vector = 3,
        Texture = 4,
    }

    internal enum ShaderDefaultTexture : int
    {
        Black = 0,
        White = 1
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

    internal class ShaderPassConstantBuffer : INativeMarshal<ShaderPassConstantBuffer, ShaderPassConstantBuffer.Native>
    {
        public string Name = string.Empty;
        public uint ShaderRegister;
        public uint RegisterSpace;
        public uint Size;

        [StructLayout(LayoutKind.Sequential)]
        internal struct Native
        {
            public nint Name;
            public uint ShaderRegister;
            public uint RegisterSpace;
            public uint Size;
        }

        public static ShaderPassConstantBuffer FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
            Size = native.Size,
        };

        public static void ToNative(ShaderPassConstantBuffer value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
            native.Size = value.Size;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }

        public static unsafe int SizeOfNative()
        {
            return sizeof(Native);
        }
    }

    internal class ShaderPassSampler : INativeMarshal<ShaderPassSampler, ShaderPassSampler.Native>
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

        public static ShaderPassSampler FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegister = native.ShaderRegister,
            RegisterSpace = native.RegisterSpace,
        };

        public static void ToNative(ShaderPassSampler value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.ShaderRegister = value.ShaderRegister;
            native.RegisterSpace = value.RegisterSpace;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }

        public static unsafe int SizeOfNative()
        {
            return sizeof(Native);
        }
    }

    internal class ShaderPassMaterialProperty : INativeMarshal<ShaderPassMaterialProperty, ShaderPassMaterialProperty.Native>
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

        public static ShaderPassMaterialProperty FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            Offset = native.Offset,
            Size = native.Size,
        };

        public static void ToNative(ShaderPassMaterialProperty value, out Native native)
        {
            native.Name = NativeString.New(value.Name);
            native.Offset = value.Offset;
            native.Size = value.Size;
        }

        public static void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);
        }

        public static unsafe int SizeOfNative()
        {
            return sizeof(Native);
        }
    }

    internal class ShaderPassTextureProperty : INativeMarshal<ShaderPassTextureProperty, ShaderPassTextureProperty.Native>
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

        public static ShaderPassTextureProperty FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            ShaderRegisterTexture = native.ShaderRegisterTexture,
            RegisterSpaceTexture = native.RegisterSpaceTexture,
            HasSampler = native.HasSampler,
            ShaderRegisterSampler = native.ShaderRegisterSampler,
            RegisterSpaceSampler = native.RegisterSpaceSampler,
        };

        public static void ToNative(ShaderPassTextureProperty value, out Native native)
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

        public static unsafe int SizeOfNative()
        {
            return sizeof(Native);
        }
    }

    internal enum ShaderPassCullMode : int
    {
        Off = 0,
        Front = 1,
        Back = 2,
    }

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

    internal enum ShaderPassBlendOp : int
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    }

    [Flags]
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
    internal struct ShaderPassBlendFormula
    {
        public ShaderPassBlend Src;
        public ShaderPassBlend Dest;
        public ShaderPassBlendOp Op;

        public static readonly ShaderPassBlendFormula Default = new()
        {
            Src = ShaderPassBlend.One,
            Dest = ShaderPassBlend.Zero,
            Op = ShaderPassBlendOp.Add
        };
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct ShaderPassBlendState
    {
        public bool Enable;
        public ShaderPassColorWriteMask WriteMask;
        public ShaderPassBlendFormula Rgb;
        public ShaderPassBlendFormula Alpha;

        public static readonly ShaderPassBlendState Default = new()
        {
            Enable = false,
            WriteMask = ShaderPassColorWriteMask.All,
            Rgb = ShaderPassBlendFormula.Default,
            Alpha = ShaderPassBlendFormula.Default
        };
    }

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

    internal class ShaderPass : INativeMarshal<ShaderPass, ShaderPass.Native>
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

            public NativeArray<byte> VertexShader;
            public NativeArray<byte> PixelShader;

            public NativeArrayMarshal<ShaderPassConstantBuffer> ConstantBuffers;
            public NativeArrayMarshal<ShaderPassSampler> Samplers;
            public NativeArrayMarshal<ShaderPassMaterialProperty> MaterialProperties;
            public NativeArrayMarshal<ShaderPassTextureProperty> TextureProperties;

            public ShaderPassCullMode Cull;
            public NativeArray<ShaderPassBlendState> Blends;
            public ShaderPassDepthState DepthState;
            public ShaderPassStencilState StencilState;
        }

        public static unsafe ShaderPass FromNative(ref Native native) => new()
        {
            Name = NativeString.Get(native.Name),
            Cull = native.Cull,
            DepthState = native.DepthState,
            StencilState = native.StencilState,
            VertexShader = native.VertexShader.Value,
            PixelShader = native.PixelShader.Value,

            ConstantBuffers = native.ConstantBuffers.Value,
            Samplers = native.Samplers.Value,
            MaterialProperties = native.MaterialProperties.Value,
            TextureProperties = native.TextureProperties.Value,

            Blends = native.Blends.Value
        };

        public static unsafe void ToNative(ShaderPass value, out Native native)
        {
            native.Name = NativeString.New(value.Name);

            native.VertexShader = value.VertexShader;
            native.PixelShader = value.PixelShader;

            native.ConstantBuffers = value.ConstantBuffers;
            native.Samplers = value.Samplers;
            native.MaterialProperties = value.MaterialProperties;
            native.TextureProperties = value.TextureProperties;

            native.Cull = value.Cull;
            native.Blends = value.Blends;
            native.DepthState = value.DepthState;
            native.StencilState = value.StencilState;
        }

        public static unsafe void FreeNative(ref Native native)
        {
            NativeString.Free(native.Name);

            native.VertexShader.Dispose();
            native.PixelShader.Dispose();

            native.ConstantBuffers.Dispose();
            native.Samplers.Dispose();
            native.MaterialProperties.Dispose();
            native.TextureProperties.Dispose();

            native.Blends.Dispose();
        }

        public static unsafe int SizeOfNative()
        {
            return sizeof(Native);
        }
    }

    enum ShaderProgramType : int
    {
        Vertex = 0,
        Pixel = 1,
    };

    public unsafe partial class Shader : NativeEngineObject
    {
        [JsonProperty]
        internal string Name { get; set; } = string.Empty;

        [JsonProperty]
        internal ShaderProperty[] Properties { get; set; } = [];

        [JsonProperty]
        internal ShaderPass[] Passes
        {
            get
            {
                int count = Shader_GetPassCount(NativePtr);
                using var buffer = new NativeArrayMarshal<ShaderPass>(count);
                Shader_GetPasses(NativePtr, buffer);
                return buffer.Value;
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

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            UploadPropertyDataToNative();

            for (int i = 0; i < GetPassCount(); i++)
            {
                CreatePassRootSignature(i);
            }
        }

        internal void UploadPropertyDataToNative()
        {
            Shader_ClearProperties(NativePtr);

            foreach (ShaderProperty prop in Properties)
            {
                ShaderProperty.ToNative(prop, out ShaderProperty.Native native);
                Shader_SetProperty(NativePtr, &native);
            }
        }

        internal int GetPassCount() => Shader_GetPassCount(NativePtr);

        internal void CompilePass(int passIndex, string filename, string program, string entrypoint, string shaderModel, ShaderProgramType programType)
        {
            using NativeString f = filename;
            using NativeString p = program;
            using NativeString e = entrypoint;
            using NativeString s = shaderModel;

            Shader_CompilePass(NativePtr, passIndex, f.Data, p.Data, e.Data, s.Data, programType);
        }

        internal void CreatePassRootSignature(int passIndex)
        {
            Shader_CreatePassRootSignature(NativePtr, passIndex);
        }

        #region Native

        [NativeFunction]
        private static partial nint Shader_New();

        [NativeFunction]
        private static partial void Shader_Delete(nint pShader);

        [NativeFunction]
        private static partial void Shader_ClearProperties(nint pShader);

        [NativeFunction]
        private static partial void Shader_SetProperty(nint pShader, ShaderProperty.Native* prop);

        [NativeFunction]
        private static partial int Shader_GetPassCount(nint pShader);

        [NativeFunction]
        private static partial void Shader_GetPasses(nint pShader, NativeArrayMarshal<ShaderPass> passes);

        [NativeFunction]
        private static partial void Shader_SetPasses(nint pShader, NativeArrayMarshal<ShaderPass> passes);

        [NativeFunction]
        private static partial void Shader_CompilePass(nint pShader, int passIndex,
            nint filename, nint program, nint entrypoint, nint shaderModel, ShaderProgramType programType);

        [NativeFunction]
        private static partial void Shader_CreatePassRootSignature(nint pShader, int passIndex);

        #endregion
    }
}
