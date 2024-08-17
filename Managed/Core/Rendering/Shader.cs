using Newtonsoft.Json;
using System.Numerics;

namespace DX12Demo.Core.Rendering
{
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
        Black = 0,
        White = 1
    }

    internal record class ShaderProperty
    {
        public string Name = string.Empty;
        public string Label = string.Empty;
        public string Tooltip = string.Empty;

        public ShaderPropertyType Type;
        public float DefaultFloatValue = 0;
        public int DefaultIntValue = 0;
        public Color DefaultColorValue = Color.White;
        public Vector4 DefaultVectorValue = Vector4.Zero;
        public ShaderDefaultTexture DefaultTextureValue = ShaderDefaultTexture.Black;
    }

    internal class ShaderPassConstantBufferProperty
    {
        public string Name = string.Empty;
        public ShaderPropertyType Type;
        public int Offset;
    }

    internal class ShaderPassTextureProperty
    {
        public string Name = string.Empty;
        public int ShaderRegisterTexture;
        public int ShaderRegisterSampler;
    }

    internal enum ShaderPassCullMode
    {
        None = 0,
        Front = 1,
        Back = 2,
    }

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

    internal enum ShaderPassBlendOp
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    }

    [Flags]
    internal enum ShaderPassColorWriteMask
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    }

    internal class ShaderPassBlendState
    {
        public bool Enabled = false;

        public ShaderPassBlend SrcBlend = ShaderPassBlend.SrcAlpha;
        public ShaderPassBlend DestBlend = ShaderPassBlend.InvSrcAlpha;
        public ShaderPassBlendOp BlendOp = ShaderPassBlendOp.Add;

        public ShaderPassBlend SrcBlendAlpha = ShaderPassBlend.One;
        public ShaderPassBlend DestBlendAlpha = ShaderPassBlend.Zero;
        public ShaderPassBlendOp BlendOpAlpha = ShaderPassBlendOp.Add;

        public ShaderPassColorWriteMask WriteMask = ShaderPassColorWriteMask.All;
    }

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

    internal class ShaderPassStencil
    {
        public ShaderPassCompareFunc StencilFunc = ShaderPassCompareFunc.Always;
        public ShaderPassStencilOp StencilPassOp = ShaderPassStencilOp.Keep;
        public ShaderPassStencilOp StencilFailOp = ShaderPassStencilOp.Keep;
        public ShaderPassStencilOp StencilDepthFailOp = ShaderPassStencilOp.Keep;
    }

    internal class ShaderPassDepthStencilState
    {
        public bool DepthEnabled = true;
        public bool DepthWrite = true;
        public ShaderPassCompareFunc DepthFunc = ShaderPassCompareFunc.Less;

        public bool StencilEnabled = false;
        public byte StencilReadMask = 0xFF;
        public byte StencilWriteMask = 0xFF;
        public ShaderPassStencil StencilFrontFace = new();
        public ShaderPassStencil StencilBackFace = new();
    }

    internal class ShaderPass
    {
        public string Name = "PassName";
        public byte[] VertexShader = [];
        public byte[] PixelShader = [];
        public List<ShaderPassConstantBufferProperty> ConstantBufferProperties = [];
        public List<ShaderPassTextureProperty> TextureProperties = [];
        public ShaderPassCullMode Cull = ShaderPassCullMode.Back;
        public List<ShaderPassBlendState> Blends = [new ShaderPassBlendState()];
        public ShaderPassDepthStencilState DepthStencilState = new();
    }

    public class Shader : EngineObject
    {
        [JsonProperty]
        internal string Name { get; set; } = "ShaderName";

        [JsonProperty]
        internal List<ShaderProperty> Properties { get; set; } = [];

        [JsonProperty]
        internal List<ShaderPass> Passes { get; set; } = [];
    }
}
