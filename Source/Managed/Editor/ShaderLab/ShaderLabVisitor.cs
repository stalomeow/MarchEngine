using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using March.Core;
using March.Core.Rendering;
using March.Editor.ShaderLab.Internal;
using System.Numerics;
using System.Text.RegularExpressions;

namespace March.Editor.ShaderLab
{
    internal class ParsedShaderProperty
    {
        public string? Name;
        public string? Label;
        public string Tooltip = string.Empty;
        public List<ShaderPropertyAttribute> Attributes = [];
        public ShaderPropertyType Type;

        public float DefaultFloat;
        public int DefaultInt;
        public Color DefaultColor;
        public Vector4 DefaultVector;
        public ShaderDefaultTexture DefaultTexture;
    }

    internal class ParsedShaderPass
    {
        public string Name = "<Unnamed Pass>";
        public string ShaderModel = "6.0";
        public string? HlslProgram;
        public string? VsEntrypoint;
        public string? PsEntrypoint;
        public ShaderPassCullMode Cull = ShaderPassCullMode.Back;
        public ShaderPassCompareFunc? ZTest = ShaderPassCompareFunc.Less;
        public bool ZWrite = true;
        public Dictionary<int, ShaderPassBlendState> Blends = [];
        public ShaderPassStencilState Stencil = new();
    }

    internal class ParsedShaderData
    {
        public string? Name;
        public List<ParsedShaderProperty> Properties = [];
        public List<ParsedShaderPass> Passes = [];
    }

    internal sealed partial class ShaderLabVisitor : ShaderLabBaseVisitor<int>
    {
        public ParsedShaderData Data { get; } = new();

        private int m_CurrentPassIndex = -1;

        private ParsedShaderPass CurrentPass => Data.Passes[m_CurrentPassIndex];

        public override int VisitShader([NotNull] ShaderLabParser.ShaderContext context)
        {
            Data.Name = context.StringLiteral().GetText()[1..^1]; // remove quotes
            return base.VisitShader(context);
        }

        public override int VisitPropertyDeclaration([NotNull] ShaderLabParser.PropertyDeclarationContext context)
        {
            ParsedShaderProperty prop = new();

            var i = context.GetText();
            var a = context.Identifier().GetText();
            var b = context.StringLiteral().GetText();

            prop.Name = context.Identifier().GetText();
            prop.Label = context.StringLiteral().GetText()[1..^1]; // remove quotes

            var type = context.propertyTypeDeclaration();
            Visit(type);

            prop.Type = type.GetChild<ITerminalNode>(0).Symbol.Type switch
            {
                ShaderLabParser.Float => ShaderPropertyType.Float,
                ShaderLabParser.Int => ShaderPropertyType.Int,
                ShaderLabParser.Color => ShaderPropertyType.Color,
                ShaderLabParser.Vector => ShaderPropertyType.Vector,
                ShaderLabParser.Texture => ShaderPropertyType.Texture,
                _ => throw new NotSupportedException("Unknown property type"),
            };

            foreach (var attr in context.attributeDeclaration())
            {
                Visit(attr);

                string rawText = attr.BracketLiteral().GetText()[1..^1]; // remove brackets
                Match m = AttributeRegex().Match(rawText);

                if (m.Success)
                {
                    string name = m.Groups[1].Value.Trim();
                    string? arguments = m.Groups.Count > 2 ? m.Groups[3].Value.Trim() : null;

                    prop.Attributes.Add(new()
                    {
                        Name = name,
                        Arguments = arguments
                    });

                    if (name == "Tooltip")
                    {
                        prop.Tooltip = arguments ?? string.Empty;
                    }
                }
            }

            var defaultValue = context.propertyDefaultValueExpression();
            Visit(defaultValue);

            switch (prop.Type)
            {
                case ShaderPropertyType.Float:
                    prop.DefaultFloat = float.Parse(defaultValue.numberLiteralExpression().GetText());
                    break;

                case ShaderPropertyType.Int:
                    prop.DefaultInt = int.Parse(defaultValue.numberLiteralExpression().GetText());
                    break;

                case ShaderPropertyType.Color:
                    prop.DefaultColor = ParseColor(defaultValue.vectorLiteralExpression());
                    break;

                case ShaderPropertyType.Vector:
                    prop.DefaultVector = ParseVector(defaultValue.vectorLiteralExpression());
                    break;

                case ShaderPropertyType.Texture:
                    prop.DefaultTexture = ParseTexture(defaultValue.textureLiteralExpression());
                    break;
            }

            Data.Properties.Add(prop);
            return 0;
        }

        public override int VisitPassBlock([NotNull] ShaderLabParser.PassBlockContext context)
        {
            Data.Passes.Add(new ParsedShaderPass());
            m_CurrentPassIndex = Data.Passes.Count - 1;
            var result = base.VisitPassBlock(context);
            m_CurrentPassIndex = -1;
            return result;
        }

        public override int VisitNameDeclaration([NotNull] ShaderLabParser.NameDeclarationContext context)
        {
            CurrentPass.Name = context.StringLiteral().GetText()[1..^1]; // remove quotes
            return base.VisitNameDeclaration(context);
        }

        public override int VisitCullDeclaration([NotNull] ShaderLabParser.CullDeclarationContext context)
        {
            CurrentPass.Cull = context.cullModeValue().GetChild<ITerminalNode>(0).Symbol.Type switch
            {
                ShaderLabParser.Back => ShaderPassCullMode.Back,
                ShaderLabParser.Front => ShaderPassCullMode.Front,
                ShaderLabParser.Off => ShaderPassCullMode.Off,
                _ => throw new NotSupportedException("Unknown cull mode"),
            };
            return base.VisitCullDeclaration(context);
        }

        public override int VisitZTestDeclaration([NotNull] ShaderLabParser.ZTestDeclarationContext context)
        {
            if (context.Off() != null)
            {
                CurrentPass.ZTest = null;
            }
            else
            {
                CurrentPass.ZTest = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            }

            return base.VisitZTestDeclaration(context);
        }

        public override int VisitZWriteDeclaration([NotNull] ShaderLabParser.ZWriteDeclarationContext context)
        {
            if (context.Off() != null)
            {
                CurrentPass.ZWrite = false;
            }
            else
            {
                CurrentPass.ZWrite = true;
            }

            return base.VisitZWriteDeclaration(context);
        }

        public override int VisitBlendDeclaration([NotNull] ShaderLabParser.BlendDeclarationContext context)
        {
            int rtvIndex = int.Parse(context.IntegerLiteral().GetText());
            ShaderPassBlendState blend = CurrentPass.Blends.GetValueOrDefault(rtvIndex, ShaderPassBlendState.Default);

            if (context.Off() != null)
            {
                blend.Enable = false;
            }
            else
            {
                blend.Enable = true;
                blend.Rgb.Src = ParseBlendFactor(context.blendFactorValue(0).GetChild<ITerminalNode>(0));
                blend.Rgb.Dest = ParseBlendFactor(context.blendFactorValue(1).GetChild<ITerminalNode>(0));
                blend.Alpha.Src = ParseBlendFactor(context.blendFactorValue(2).GetChild<ITerminalNode>(0));
                blend.Alpha.Dest = ParseBlendFactor(context.blendFactorValue(3).GetChild<ITerminalNode>(0));
            }

            CurrentPass.Blends[rtvIndex] = blend;
            return base.VisitBlendDeclaration(context);
        }

        public override int VisitBlendOpDeclaration([NotNull] ShaderLabParser.BlendOpDeclarationContext context)
        {
            int rtvIndex = int.Parse(context.IntegerLiteral().GetText());
            ShaderPassBlendState blend = CurrentPass.Blends.GetValueOrDefault(rtvIndex, ShaderPassBlendState.Default);

            blend.Rgb.Op = ParseBlendOp(context.blendOpValue(0).GetChild<ITerminalNode>(0));
            blend.Alpha.Op = ParseBlendOp(context.blendOpValue(1).GetChild<ITerminalNode>(0));

            CurrentPass.Blends[rtvIndex] = blend;
            return base.VisitBlendOpDeclaration(context);
        }

        public override int VisitColorMaskDeclaration([NotNull] ShaderLabParser.ColorMaskDeclarationContext context)
        {
            int rtvIndex = int.Parse(context.IntegerLiteral(0).GetText());
            ShaderPassBlendState blend = CurrentPass.Blends.GetValueOrDefault(rtvIndex, ShaderPassBlendState.Default);

            if (context.IntegerLiteral(1) != null)
            {
                if (context.IntegerLiteral(1).GetText() != "0")
                {
                    throw new NotSupportedException("Invalid color write mask");
                }

                blend.WriteMask = ShaderPassColorWriteMask.None;
            }
            else
            {
                blend.WriteMask = ShaderPassColorWriteMask.None;

                string writeMask = context.Identifier().GetText();
                HashSet<char> chars = [];

                foreach (char c in writeMask.ToLower())
                {
                    if (!chars.Add(c) || (c != 'r' && c != 'g' && c != 'b' && c != 'a'))
                    {
                        throw new NotSupportedException("Invalid color write mask");
                    }

                    switch (c)
                    {
                        case 'r': blend.WriteMask |= ShaderPassColorWriteMask.Red; break;
                        case 'g': blend.WriteMask |= ShaderPassColorWriteMask.Green; break;
                        case 'b': blend.WriteMask |= ShaderPassColorWriteMask.Blue; break;
                        case 'a': blend.WriteMask |= ShaderPassColorWriteMask.Alpha; break;
                    }
                }
            }

            CurrentPass.Blends[rtvIndex] = blend;
            return base.VisitColorMaskDeclaration(context);
        }

        public override int VisitStencilBlock([NotNull] ShaderLabParser.StencilBlockContext context)
        {
            CurrentPass.Stencil.Enable = true;
            return base.VisitStencilBlock(context);
        }

        public override int VisitStencilReadMaskDeclaration([NotNull] ShaderLabParser.StencilReadMaskDeclarationContext context)
        {
            int numBase = 10;
            string mask = context.IntegerLiteral().GetText();

            if (mask.StartsWith("0x"))
            {
                numBase = 16;
                mask = mask[2..];
            }

            CurrentPass.Stencil.ReadMask = Convert.ToByte(mask, numBase);
            return base.VisitStencilReadMaskDeclaration(context);
        }

        public override int VisitStencilWriteMaskDeclaration([NotNull] ShaderLabParser.StencilWriteMaskDeclarationContext context)
        {
            int numBase = 10;
            string mask = context.IntegerLiteral().GetText();

            if (mask.StartsWith("0x"))
            {
                numBase = 16;
                mask = mask[2..];
            }

            CurrentPass.Stencil.WriteMask = Convert.ToByte(mask, numBase);
            return base.VisitStencilWriteMaskDeclaration(context);
        }

        public override int VisitStencilCompFrontDeclaration([NotNull] ShaderLabParser.StencilCompFrontDeclarationContext context)
        {
            CurrentPass.Stencil.FrontFace.Compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilCompFrontDeclaration(context);
        }

        public override int VisitStencilPassFrontDeclaration([NotNull] ShaderLabParser.StencilPassFrontDeclarationContext context)
        {
            CurrentPass.Stencil.FrontFace.PassOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilPassFrontDeclaration(context);
        }

        public override int VisitStencilFailFrontDeclaration([NotNull] ShaderLabParser.StencilFailFrontDeclarationContext context)
        {
            CurrentPass.Stencil.FrontFace.FailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilFailFrontDeclaration(context);
        }

        public override int VisitStencilZFailFrontDeclaration([NotNull] ShaderLabParser.StencilZFailFrontDeclarationContext context)
        {
            CurrentPass.Stencil.FrontFace.DepthFailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilZFailFrontDeclaration(context);
        }

        public override int VisitStencilCompBackDeclaration([NotNull] ShaderLabParser.StencilCompBackDeclarationContext context)
        {
            CurrentPass.Stencil.BackFace.Compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilCompBackDeclaration(context);
        }

        public override int VisitStencilPassBackDeclaration([NotNull] ShaderLabParser.StencilPassBackDeclarationContext context)
        {
            CurrentPass.Stencil.BackFace.PassOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilPassBackDeclaration(context);
        }

        public override int VisitStencilFailBackDeclaration([NotNull] ShaderLabParser.StencilFailBackDeclarationContext context)
        {
            CurrentPass.Stencil.BackFace.FailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilFailBackDeclaration(context);
        }

        public override int VisitStencilZFailBackDeclaration([NotNull] ShaderLabParser.StencilZFailBackDeclarationContext context)
        {
            CurrentPass.Stencil.BackFace.DepthFailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilZFailBackDeclaration(context);
        }

        public override int VisitHlslProgramDeclaration([NotNull] ShaderLabParser.HlslProgramDeclarationContext context)
        {
            string code = context.HlslProgram().GetText()[11..^7].Trim(); // Remove HLSLPROGRAM and ENDHLSL
            CurrentPass.HlslProgram = code;

            foreach (string line in code.Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries))
            {
                string[] segments = line.Split([' ', '\t'], StringSplitOptions.RemoveEmptyEntries);

                // #pragma command ...args，至少要 2 个 segments
                if (segments.Length >= 2 && segments[0] == "#pragma")
                {
                    switch (segments[1])
                    {
                        case "target":
                            CurrentPass.ShaderModel = MustGet(segments, 2, "Missing shader model version");
                            break;
                        case "vs":
                            CurrentPass.VsEntrypoint = MustGet(segments, 2, "Missing vertex shader entrypoint");
                            break;
                        case "ps":
                            CurrentPass.PsEntrypoint = MustGet(segments, 2, "Missing pixel shader entrypoint");
                            break;
                        default:
                            // 未知指令，忽略，可能是 hlsl 编译器的指令
                            break;
                    }
                }
            }

            return base.VisitHlslProgramDeclaration(context);

            static string MustGet(string[] arr, int i, string error)
            {
                if (i >= arr.Length)
                {
                    throw new InvalidOperationException(error);
                }

                return arr[i];
            }
        }

        private static Color ParseColor(ShaderLabParser.VectorLiteralExpressionContext context)
        {
            return new Color(
                float.Parse(context.numberLiteralExpression(0).GetText()),
                float.Parse(context.numberLiteralExpression(1).GetText()),
                float.Parse(context.numberLiteralExpression(2).GetText()),
                float.Parse(context.numberLiteralExpression(3).GetText())
            );
        }

        private static Vector4 ParseVector(ShaderLabParser.VectorLiteralExpressionContext context)
        {
            return new Vector4(
                float.Parse(context.numberLiteralExpression(0).GetText()),
                float.Parse(context.numberLiteralExpression(1).GetText()),
                float.Parse(context.numberLiteralExpression(2).GetText()),
                float.Parse(context.numberLiteralExpression(3).GetText())
            );
        }

        private static ShaderDefaultTexture ParseTexture(ShaderLabParser.TextureLiteralExpressionContext context)
        {
            string value = context.StringLiteral().GetText()[1..^1]; // Remove quotes
            return value switch
            {
                "black" => ShaderDefaultTexture.Black,
                "white" => ShaderDefaultTexture.White,
                _ => throw new NotSupportedException("Unknown texture value")
            };
        }

        private static ShaderPassCompareFunc ParseCompareFunc(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Never => ShaderPassCompareFunc.Never,
                ShaderLabParser.Less => ShaderPassCompareFunc.Less,
                ShaderLabParser.Equal => ShaderPassCompareFunc.Equal,
                ShaderLabParser.LessEqual => ShaderPassCompareFunc.LessEqual,
                ShaderLabParser.Greater => ShaderPassCompareFunc.Greater,
                ShaderLabParser.NotEqual => ShaderPassCompareFunc.NotEqual,
                ShaderLabParser.GreaterEqual => ShaderPassCompareFunc.GreaterEqual,
                ShaderLabParser.Always => ShaderPassCompareFunc.Always,
                _ => throw new NotSupportedException("Unknown comparison function"),
            };
        }

        private static ShaderPassBlend ParseBlendFactor(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Zero => ShaderPassBlend.Zero,
                ShaderLabParser.One => ShaderPassBlend.One,
                ShaderLabParser.SrcColor => ShaderPassBlend.SrcColor,
                ShaderLabParser.InvSrcColor => ShaderPassBlend.InvSrcColor,
                ShaderLabParser.SrcAlpha => ShaderPassBlend.SrcAlpha,
                ShaderLabParser.InvSrcAlpha => ShaderPassBlend.InvSrcAlpha,
                ShaderLabParser.DestAlpha => ShaderPassBlend.DestAlpha,
                ShaderLabParser.InvDestAlpha => ShaderPassBlend.InvDestAlpha,
                ShaderLabParser.DestColor => ShaderPassBlend.DestColor,
                ShaderLabParser.InvDestColor => ShaderPassBlend.InvDestColor,
                ShaderLabParser.SrcAlphaSat => ShaderPassBlend.SrcAlphaSat,
                _ => throw new NotSupportedException("Unknown blend factor"),
            };
        }

        private static ShaderPassBlendOp ParseBlendOp(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Add => ShaderPassBlendOp.Add,
                ShaderLabParser.Sub => ShaderPassBlendOp.Subtract,
                ShaderLabParser.RevSub => ShaderPassBlendOp.RevSubtract,
                ShaderLabParser.Min => ShaderPassBlendOp.Min,
                ShaderLabParser.Max => ShaderPassBlendOp.Max,
                _ => throw new NotSupportedException("Unknown blend operation"),
            };
        }

        private static ShaderPassStencilOp ParseStencilOp(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Keep => ShaderPassStencilOp.Keep,
                ShaderLabParser.Zero => ShaderPassStencilOp.Zero,
                ShaderLabParser.Replace => ShaderPassStencilOp.Replace,
                ShaderLabParser.IncrSat => ShaderPassStencilOp.IncrSat,
                ShaderLabParser.DecrSat => ShaderPassStencilOp.DecrSat,
                ShaderLabParser.Invert => ShaderPassStencilOp.Invert,
                ShaderLabParser.Incr => ShaderPassStencilOp.Incr,
                ShaderLabParser.Decr => ShaderPassStencilOp.Decr,
                _ => throw new NotSupportedException("Unknown stencil operation"),
            };
        }

        [GeneratedRegex(@"^(.+?)(\((.*)\))?$")]
        private static partial Regex AttributeRegex();
    }
}
