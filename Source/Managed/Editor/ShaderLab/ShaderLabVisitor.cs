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

    internal struct ParsedBlend
    {
        public bool Enable;
        public ShaderPassBlend SrcRgb;
        public ShaderPassBlend DstRgb;
        public ShaderPassBlend SrcAlpha;
        public ShaderPassBlend DstAlpha;

        public static readonly ParsedBlend Default = new()
        {
            Enable = false,
            SrcRgb = ShaderPassBlend.One,
            DstRgb = ShaderPassBlend.Zero,
            SrcAlpha = ShaderPassBlend.One,
            DstAlpha = ShaderPassBlend.Zero,
        };
    }

    internal struct ParsedBlendOp
    {
        public ShaderPassBlendOp OpRgb;
        public ShaderPassBlendOp OpAlpha;

        public static readonly ParsedBlendOp Default = new()
        {
            OpRgb = ShaderPassBlendOp.Add,
            OpAlpha = ShaderPassBlendOp.Add,
        };
    }

    internal abstract class ParsedMetadataContainer
    {
        private ShaderPassCullMode? m_Cull;

        private bool? m_ZWrite;
        private bool? m_IsZTestDisabled;
        private ShaderPassCompareFunc? m_ZTest;

        private ParsedBlend[]? m_Blends;
        private ParsedBlendOp[]? m_BlendOps;
        private ShaderPassColorWriteMask[]? m_ColorMasks;

        private bool m_IsStencilSet = false;
        private ShaderPassStencilState m_Stencil = ShaderPassStencilState.Default;

        private readonly Dictionary<string, string> m_Tags = [];

        public void SetCull(ShaderPassCullMode cull)
        {
            m_Cull = cull;
        }

        public void SetZWrite(bool zWrite)
        {
            m_ZWrite = zWrite;
        }

        public void SetZTest(ShaderPassCompareFunc zTest)
        {
            m_IsZTestDisabled = false;
            m_ZTest = zTest;
        }

        public void DisableZTest()
        {
            m_IsZTestDisabled = true;
            m_ZTest = ShaderPassCompareFunc.Always;
        }

        public ParsedBlend[] SetBlends()
        {
            return m_Blends ??= Enumerable.Repeat(ParsedBlend.Default, 8).ToArray();
        }

        public ParsedBlendOp[] SetBlendOps()
        {
            return m_BlendOps ??= Enumerable.Repeat(ParsedBlendOp.Default, 8).ToArray();
        }

        public ShaderPassColorWriteMask[] SetColorMasks()
        {
            return m_ColorMasks ??= Enumerable.Repeat(ShaderPassColorWriteMask.All, 8).ToArray();
        }

        public ref ShaderPassStencilState SetStencilState()
        {
            m_IsStencilSet = true;
            return ref m_Stencil;
        }

        public void SetTag(string key, string value)
        {
            m_Tags[key] = value;
        }

        public ShaderPassCullMode GetCull(ParsedMetadataContainer parent)
        {
            return m_Cull ?? parent.m_Cull ?? ShaderPassCullMode.Back;
        }

        public ShaderPassDepthState GetDepthState(ParsedMetadataContainer parent)
        {
            if (m_IsZTestDisabled ?? parent.m_IsZTestDisabled ?? false)
            {
                // 禁用 ZTest 时，ZWrite 默认为 Off
                bool write = m_ZWrite ?? parent.m_ZWrite ?? false;

                // https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
                // Set DepthEnable to FALSE to disable depth testing and prevent writing to the depth buffer.
                return new ShaderPassDepthState
                {
                    Enable = write, // 如果强制要求写入深度，则启用深度测试，否则写不进去
                    Write = write,
                    Compare = ShaderPassCompareFunc.Always, // 深度测试关闭时，默认通过
                };
            }
            else
            {
                return new ShaderPassDepthState
                {
                    Enable = true,
                    Write = m_ZWrite ?? parent.m_ZWrite ?? true,
                    Compare = m_ZTest ?? parent.m_ZTest ?? ShaderPassCompareFunc.Less,
                };
            }
        }

        public ShaderPassBlendState[] GetBlendStates(ParsedMetadataContainer parent)
        {
            ShaderPassBlendState[] blends = Enumerable.Repeat(ShaderPassBlendState.Default, 8).ToArray();

            ParsedBlend[]? parsedBlends = m_Blends ?? parent.m_Blends;
            ParsedBlendOp[]? parsedOps = m_BlendOps ?? parent.m_BlendOps;
            ShaderPassColorWriteMask[]? writeMasks = m_ColorMasks ?? parent.m_ColorMasks;

            if (parsedBlends != null)
            {
                for (int i = 0; i < blends.Length; i++)
                {
                    blends[i].Enable = parsedBlends[i].Enable;
                    blends[i].Rgb.Src = parsedBlends[i].SrcRgb;
                    blends[i].Rgb.Dest = parsedBlends[i].DstRgb;
                    blends[i].Alpha.Src = parsedBlends[i].SrcAlpha;
                    blends[i].Alpha.Dest = parsedBlends[i].DstAlpha;
                }
            }

            if (parsedOps != null)
            {
                for (int i = 0; i < blends.Length; i++)
                {
                    blends[i].Rgb.Op = parsedOps[i].OpRgb;
                    blends[i].Alpha.Op = parsedOps[i].OpAlpha;
                }
            }

            if (writeMasks != null)
            {
                for (int i = 0; i < blends.Length; i++)
                {
                    blends[i].WriteMask = writeMasks[i];
                }
            }

            if (blends.All(b => b.Equals(blends[0])))
            {
                return blends[..1]; // 所有的都一样，所以只保留一个
            }
            else
            {
                return blends;
            }
        }

        public ShaderPassStencilState GetStencilState(ParsedMetadataContainer parent)
        {
            if (m_IsStencilSet)
            {
                return m_Stencil;
            }

            if (parent.m_IsStencilSet)
            {
                return parent.m_Stencil;
            }

            return ShaderPassStencilState.Default;
        }

        public ShaderPassTag[] GetTags(ParsedMetadataContainer parent)
        {
            Dictionary<string, string> results = new(parent.m_Tags);

            foreach (KeyValuePair<string, string> kv in m_Tags)
            {
                results[kv.Key] = kv.Value;
            }

            return results.Select(kv => new ShaderPassTag(kv.Key, kv.Value)).OrderBy(tag => tag.Key).ToArray();
        }
    }

    internal class ParsedShaderPass : ParsedMetadataContainer
    {
        public string Name = "UnnamedPass";
        public string? HlslProgram;
    }

    internal class ParsedShaderData : ParsedMetadataContainer
    {
        public string? Name;
        public List<ParsedShaderProperty> Properties = [];
        public List<string> HlslIncludes = [];
        public List<ParsedShaderPass> Passes = [];

        public string GetSourceCode(int passIndex)
        {
            if (passIndex < 0 || passIndex >= Passes.Count)
            {
                return string.Empty;
            }

            ParsedShaderPass pass = Passes[passIndex];
            string code = string.Join("\n", HlslIncludes);

            if (pass.HlslProgram != null)
            {
                code += "\n" + pass.HlslProgram;
            }

            return code;
        }
    }

    internal sealed partial class ShaderLabVisitor(ShaderLabErrorListener errorListener) : ShaderLabBaseVisitor<int>
    {
        public ParsedShaderData Data { get; } = new();

        private int m_CurrentPassIndex = -1;

        private ParsedShaderPass CurrentPass
        {
            get
            {
                if (m_CurrentPassIndex == -1)
                {
                    throw new InvalidOperationException("Not in a pass block");
                }

                return Data.Passes[m_CurrentPassIndex];
            }
        }

        private ParsedMetadataContainer CurrentMetadataContainer
        {
            get
            {
                if (m_CurrentPassIndex == -1)
                {
                    return Data;
                }

                return Data.Passes[m_CurrentPassIndex];
            }
        }

        protected override bool ShouldVisitNextChild(IRuleNode node, int currentResult)
        {
            return !errorListener.HasError;
        }

        public override int Visit(IParseTree tree)
        {
            try
            {
                return base.Visit(tree);
            }
            catch (Exception e)
            {
                errorListener.AddException(e);
            }

            return DefaultResult;
        }

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

        public override int VisitHlslIncludeDeclaration([NotNull] ShaderLabParser.HlslIncludeDeclarationContext context)
        {
            // Remove HLSLINCLUDE and ENDHLSL，但要保留换行，不要 Trim，后面要设置行号
            string code = context.HlslInclude().GetText()[11..^7];

            // 保证行号和源文件一致
            // 注意：#line 设置的是下一行的行号
            int startLineNumber = context.HlslInclude().Symbol.Line;
            Data.HlslIncludes.Add($"#line {startLineNumber}\n" + code);

            return base.VisitHlslIncludeDeclaration(context);
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

        public override int VisitTagDeclaration([NotNull] ShaderLabParser.TagDeclarationContext context)
        {
            string key = context.StringLiteral(0).GetText()[1..^1]; // remove quotes
            string value = context.StringLiteral(1).GetText()[1..^1]; // remove quotes
            CurrentMetadataContainer.SetTag(key, value);
            return base.VisitTagDeclaration(context);
        }

        public override int VisitCullDeclaration([NotNull] ShaderLabParser.CullDeclarationContext context)
        {
            CurrentMetadataContainer.SetCull(context.cullModeValue().GetChild<ITerminalNode>(0).Symbol.Type switch
            {
                ShaderLabParser.Back => ShaderPassCullMode.Back,
                ShaderLabParser.Front => ShaderPassCullMode.Front,
                ShaderLabParser.Off => ShaderPassCullMode.Off,
                _ => throw new NotSupportedException("Unknown cull mode"),
            });

            return base.VisitCullDeclaration(context);
        }

        public override int VisitZTestDeclaration([NotNull] ShaderLabParser.ZTestDeclarationContext context)
        {
            if (context.Disabled() != null)
            {
                CurrentMetadataContainer.DisableZTest();
            }
            else
            {
                CurrentMetadataContainer.SetZTest(ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0)));
            }

            return base.VisitZTestDeclaration(context);
        }

        public override int VisitZWriteDeclaration([NotNull] ShaderLabParser.ZWriteDeclarationContext context)
        {
            CurrentMetadataContainer.SetZWrite(context.On() != null);
            return base.VisitZWriteDeclaration(context);
        }

        public override int VisitBlendDeclaration([NotNull] ShaderLabParser.BlendDeclarationContext context)
        {
            ParsedBlend[] blends = CurrentMetadataContainer.SetBlends();

            // 默认设置所有 Target
            int begin = 0;
            int end = blends.Length;

            if (context.IntegerLiteral() != null)
            {
                begin = int.Parse(context.IntegerLiteral().GetText());
                end = begin + 1;

                if (begin < 0 || begin >= blends.Length)
                {
                    throw new NotSupportedException("Invalid render target index");
                }
            }

            for (int i = begin; i < end; i++)
            {
                ref ParsedBlend blend = ref blends[i];

                if (context.Off() != null)
                {
                    blend.Enable = false;
                }
                else
                {
                    blend.Enable = true;
                    blend.SrcRgb = ParseBlendFactor(context.blendFactorValue(0).GetChild<ITerminalNode>(0));
                    blend.DstRgb = ParseBlendFactor(context.blendFactorValue(1).GetChild<ITerminalNode>(0));

                    if (context.blendFactorValue(2) != null && context.blendFactorValue(3) != null)
                    {
                        blend.SrcAlpha = ParseBlendFactor(context.blendFactorValue(2).GetChild<ITerminalNode>(0));
                        blend.DstAlpha = ParseBlendFactor(context.blendFactorValue(3).GetChild<ITerminalNode>(0));
                    }
                    else
                    {
                        blend.SrcAlpha = blend.SrcRgb;
                        blend.DstAlpha = blend.DstRgb;
                    }
                }
            }

            return base.VisitBlendDeclaration(context);
        }

        public override int VisitBlendOpDeclaration([NotNull] ShaderLabParser.BlendOpDeclarationContext context)
        {
            ParsedBlendOp[] blendOps = CurrentMetadataContainer.SetBlendOps();

            // 默认设置所有 Target
            int begin = 0;
            int end = blendOps.Length;

            if (context.IntegerLiteral() != null)
            {
                begin = int.Parse(context.IntegerLiteral().GetText());
                end = begin + 1;

                if (begin < 0 || begin >= blendOps.Length)
                {
                    throw new NotSupportedException("Invalid render target index");
                }
            }

            for (int i = begin; i < end; i++)
            {
                ref ParsedBlendOp blend = ref blendOps[i];

                blend.OpRgb = ParseBlendOp(context.blendOpValue(0).GetChild<ITerminalNode>(0));

                if (context.blendOpValue(1) != null)
                {
                    blend.OpAlpha = ParseBlendOp(context.blendOpValue(1).GetChild<ITerminalNode>(0));
                }
                else
                {
                    blend.OpAlpha = blend.OpRgb;
                }
            }

            return base.VisitBlendOpDeclaration(context);
        }

        public override int VisitColorMaskInt1Declaration([NotNull] ShaderLabParser.ColorMaskInt1DeclarationContext context)
        {
            if (context.IntegerLiteral().GetText() != "0")
            {
                throw new NotSupportedException("Invalid color write mask");
            }

            ShaderPassColorWriteMask[] colorMasks = CurrentMetadataContainer.SetColorMasks();

            for (int i = 0; i < colorMasks.Length; i++)
            {
                colorMasks[i] = ShaderPassColorWriteMask.None;
            }

            return base.VisitColorMaskInt1Declaration(context);
        }

        public override int VisitColorMaskInt2Declaration([NotNull] ShaderLabParser.ColorMaskInt2DeclarationContext context)
        {
            ShaderPassColorWriteMask[] colorMasks = CurrentMetadataContainer.SetColorMasks();

            int i = int.Parse(context.IntegerLiteral(0).GetText());

            if (i < 0 || i >= colorMasks.Length)
            {
                throw new NotSupportedException("Invalid render target index");
            }

            if (context.IntegerLiteral(1).GetText() != "0")
            {
                throw new NotSupportedException("Invalid color write mask");
            }

            colorMasks[i] = ShaderPassColorWriteMask.None;

            return base.VisitColorMaskInt2Declaration(context);
        }

        public override int VisitColorMaskIdentifierDeclaration([NotNull] ShaderLabParser.ColorMaskIdentifierDeclarationContext context)
        {
            ShaderPassColorWriteMask[] colorMasks = CurrentMetadataContainer.SetColorMasks();

            // 默认设置所有 Target
            int begin = 0;
            int end = colorMasks.Length;

            if (context.IntegerLiteral() != null)
            {
                begin = int.Parse(context.IntegerLiteral().GetText());
                end = begin + 1;

                if (begin < 0 || begin >= colorMasks.Length)
                {
                    throw new NotSupportedException("Invalid render target index");
                }
            }

            ShaderPassColorWriteMask mask = ShaderPassColorWriteMask.None;
            HashSet<char> chars = [];

            foreach (char c in context.Identifier().GetText().ToLower())
            {
                if (!chars.Add(c) || (c != 'r' && c != 'g' && c != 'b' && c != 'a'))
                {
                    throw new NotSupportedException("Invalid color write mask");
                }

                switch (c)
                {
                    case 'r': mask |= ShaderPassColorWriteMask.Red; break;
                    case 'g': mask |= ShaderPassColorWriteMask.Green; break;
                    case 'b': mask |= ShaderPassColorWriteMask.Blue; break;
                    case 'a': mask |= ShaderPassColorWriteMask.Alpha; break;
                }
            }

            for (int i = begin; i < end; i++)
            {
                colorMasks[i] = mask;
            }

            return base.VisitColorMaskIdentifierDeclaration(context);
        }

        public override int VisitStencilBlock([NotNull] ShaderLabParser.StencilBlockContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.Enable = true;
            return base.VisitStencilBlock(context);
        }

        public override int VisitStencilRefDeclaration([NotNull] ShaderLabParser.StencilRefDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.Ref = byte.Parse(context.IntegerLiteral().GetText());
            return base.VisitStencilRefDeclaration(context);
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

            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.ReadMask = Convert.ToByte(mask, numBase);
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

            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.WriteMask = Convert.ToByte(mask, numBase);
            return base.VisitStencilWriteMaskDeclaration(context);
        }

        public override int VisitStencilCompDeclaration([NotNull] ShaderLabParser.StencilCompDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            ShaderPassCompareFunc compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            stencil.FrontFace.Compare = compare;
            stencil.BackFace.Compare = compare;
            return base.VisitStencilCompDeclaration(context);
        }

        public override int VisitStencilPassDeclaration([NotNull] ShaderLabParser.StencilPassDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            ShaderPassStencilOp op = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            stencil.FrontFace.PassOp = op;
            stencil.BackFace.PassOp = op;
            return base.VisitStencilPassDeclaration(context);
        }

        public override int VisitStencilFailDeclaration([NotNull] ShaderLabParser.StencilFailDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            ShaderPassStencilOp op = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            stencil.FrontFace.FailOp = op;
            stencil.BackFace.FailOp = op;
            return base.VisitStencilFailDeclaration(context);
        }

        public override int VisitStencilZFailDeclaration([NotNull] ShaderLabParser.StencilZFailDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            ShaderPassStencilOp op = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            stencil.FrontFace.DepthFailOp = op;
            stencil.BackFace.DepthFailOp = op;
            return base.VisitStencilZFailDeclaration(context);
        }

        public override int VisitStencilCompFrontDeclaration([NotNull] ShaderLabParser.StencilCompFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.FrontFace.Compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilCompFrontDeclaration(context);
        }

        public override int VisitStencilPassFrontDeclaration([NotNull] ShaderLabParser.StencilPassFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.FrontFace.PassOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilPassFrontDeclaration(context);
        }

        public override int VisitStencilFailFrontDeclaration([NotNull] ShaderLabParser.StencilFailFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.FrontFace.FailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilFailFrontDeclaration(context);
        }

        public override int VisitStencilZFailFrontDeclaration([NotNull] ShaderLabParser.StencilZFailFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.FrontFace.DepthFailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilZFailFrontDeclaration(context);
        }

        public override int VisitStencilCompBackDeclaration([NotNull] ShaderLabParser.StencilCompBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.BackFace.Compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilCompBackDeclaration(context);
        }

        public override int VisitStencilPassBackDeclaration([NotNull] ShaderLabParser.StencilPassBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.BackFace.PassOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilPassBackDeclaration(context);
        }

        public override int VisitStencilFailBackDeclaration([NotNull] ShaderLabParser.StencilFailBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.BackFace.FailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilFailBackDeclaration(context);
        }

        public override int VisitStencilZFailBackDeclaration([NotNull] ShaderLabParser.StencilZFailBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();
            stencil.BackFace.DepthFailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            return base.VisitStencilZFailBackDeclaration(context);
        }

        public override int VisitHlslProgramDeclaration([NotNull] ShaderLabParser.HlslProgramDeclarationContext context)
        {
            // Remove HLSLPROGRAM and ENDHLSL，但要保留换行，不要 Trim，后面要设置行号
            string code = context.HlslProgram().GetText()[11..^7];

            // 保证行号和源文件一致
            // 注意：#line 设置的是下一行的行号
            int startLineNumber = context.HlslProgram().Symbol.Line;
            CurrentPass.HlslProgram = $"#line {startLineNumber}\n" + code;

            return base.VisitHlslProgramDeclaration(context);
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
                ShaderLabParser.LEqual => ShaderPassCompareFunc.LessEqual,
                ShaderLabParser.Greater => ShaderPassCompareFunc.Greater,
                ShaderLabParser.NotEqual => ShaderPassCompareFunc.NotEqual,
                ShaderLabParser.GEqual => ShaderPassCompareFunc.GreaterEqual,
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
                ShaderLabParser.OneMinusSrcColor => ShaderPassBlend.InvSrcColor,
                ShaderLabParser.SrcAlpha => ShaderPassBlend.SrcAlpha,
                ShaderLabParser.OneMinusSrcAlpha => ShaderPassBlend.InvSrcAlpha,
                ShaderLabParser.DstAlpha => ShaderPassBlend.DestAlpha,
                ShaderLabParser.OneMinusDstAlpha => ShaderPassBlend.InvDestAlpha,
                ShaderLabParser.DstColor => ShaderPassBlend.DestColor,
                ShaderLabParser.OneMinusDstColor => ShaderPassBlend.InvDestColor,
                ShaderLabParser.SrcAlphaSaturate => ShaderPassBlend.SrcAlphaSat,
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
                ShaderLabParser.IncrWrap => ShaderPassStencilOp.Incr,
                ShaderLabParser.DecrWrap => ShaderPassStencilOp.Decr,
                _ => throw new NotSupportedException("Unknown stencil operation"),
            };
        }

        [GeneratedRegex(@"^(.+?)(\((.*)\))?$")]
        private static partial Regex AttributeRegex();
    }
}
