namespace March.Core.Rendering
{
    public enum CullMode
    {
        Off = 0,
        Front = 1,
        Back = 2,
    }

    public enum BlendMode
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

    public enum BlendOp
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    }

    [Flags]
    public enum ColorWriteMask
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    }

    public enum CompareFunction
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

    public enum StencilOp
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
}
