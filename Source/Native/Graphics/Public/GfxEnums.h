#pragma once

namespace march
{
    enum class CullMode
    {
        Off = 0,
        Front = 1,
        Back = 2,
    };

    enum class BlendMode
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
    };

    enum class BlendOp
    {
        Add = 0,
        Subtract = 1,
        RevSubtract = 2,
        Min = 3,
        Max = 4,
    };

    enum class ColorWriteMask
    {
        None = 0,
        Red = 1 << 0,
        Green = 1 << 1,
        Blue = 1 << 2,
        Alpha = 1 << 3,
        All = Red | Green | Blue | Alpha
    };

    enum class CompareFunction
    {
        Never = 0,
        Less = 1,
        Equal = 2,
        LessEqual = 3,
        Greater = 4,
        NotEqual = 5,
        GreaterEqual = 6,
        Always = 7,
    };

    enum class StencilOp
    {
        Keep = 0,
        Zero = 1,
        Replace = 2,
        IncrSat = 3,
        DecrSat = 4,
        Invert = 5,
        Incr = 6,
        Decr = 7,
    };
}
