using Antlr4.Runtime.Misc;
using Antlr4.Runtime.Tree;
using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using March.Core.Rendering;
using March.ShaderLab.Internal;
using System.Numerics;
using System.Text.RegularExpressions;

namespace March.ShaderLab
{
    internal struct ParsedBlend
    {
        public bool Enable;
        public ShaderPassVar<BlendMode> SrcRgb;
        public ShaderPassVar<BlendMode> DstRgb;
        public ShaderPassVar<BlendMode> SrcAlpha;
        public ShaderPassVar<BlendMode> DstAlpha;

        public static readonly ParsedBlend Default = new()
        {
            Enable = false,
            SrcRgb = BlendMode.One,
            DstRgb = BlendMode.Zero,
            SrcAlpha = BlendMode.One,
            DstAlpha = BlendMode.Zero,
        };
    }

    internal struct ParsedBlendOp
    {
        public ShaderPassVar<BlendOp> OpRgb;
        public ShaderPassVar<BlendOp> OpAlpha;

        public static readonly ParsedBlendOp Default = new()
        {
            OpRgb = BlendOp.Add,
            OpAlpha = BlendOp.Add,
        };
    }

    internal abstract class ParsedMetadataContainer
    {
        private ShaderPassVar<CullMode>? m_Cull;

        private ShaderPassVar<bool>? m_ZWrite;
        private bool? m_IsZTestDisabled;
        private ShaderPassVar<CompareFunction>? m_ZTest;

        private ParsedBlend[]? m_Blends;
        private ParsedBlendOp[]? m_BlendOps;
        private ShaderPassVar<ColorWriteMask>[]? m_ColorMasks;

        private bool m_IsStencilSet = false;
        private ShaderPassStencilState m_Stencil = ShaderPassStencilState.Default;

        private readonly Dictionary<string, string> m_Tags = [];

        public void SetCull(CullMode cull)
        {
            m_Cull = new ShaderPassVar<CullMode>(cull);
        }

        public void SetCull(string propertyName)
        {
            m_Cull = new ShaderPassVar<CullMode>(propertyName);
        }

        public void SetZWrite(bool zWrite)
        {
            m_ZWrite = new ShaderPassVar<bool>(zWrite);
        }

        public void SetZWrite(string propertyName)
        {
            m_ZWrite = new ShaderPassVar<bool>(propertyName);
        }

        public void SetZTest(CompareFunction zTest)
        {
            m_IsZTestDisabled = false;
            m_ZTest = new ShaderPassVar<CompareFunction>(zTest);
        }

        public void SetZTest(string propertyName)
        {
            m_IsZTestDisabled = false;
            m_ZTest = new ShaderPassVar<CompareFunction>(propertyName);
        }

        public void DisableZTest()
        {
            m_IsZTestDisabled = true;
            m_ZTest = CompareFunction.Always;
        }

        public ParsedBlend[] SetBlends()
        {
            return m_Blends ??= Enumerable.Repeat(ParsedBlend.Default, 8).ToArray();
        }

        public ParsedBlendOp[] SetBlendOps()
        {
            return m_BlendOps ??= Enumerable.Repeat(ParsedBlendOp.Default, 8).ToArray();
        }

        public ShaderPassVar<ColorWriteMask>[] SetColorMasks()
        {
            return m_ColorMasks ??= Enumerable.Repeat(new ShaderPassVar<ColorWriteMask>(ColorWriteMask.All), 8).ToArray();
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

        public ShaderPassVar<CullMode> GetCull(ParsedMetadataContainer parent)
        {
            return m_Cull ?? parent.m_Cull ?? CullMode.Back;
        }

        public ShaderPassDepthState GetDepthState(ParsedMetadataContainer parent)
        {
            if (m_IsZTestDisabled ?? parent.m_IsZTestDisabled ?? false)
            {
                // 禁用 ZTest 时，ZWrite 默认为 Off
                ShaderPassVar<bool> write = m_ZWrite ?? parent.m_ZWrite ?? false;

                // https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-depth-stencil
                // Set DepthEnable to FALSE to disable depth testing and prevent writing to the depth buffer.
                return new ShaderPassDepthState
                {
                    Enable = write.PropertyId.HasValue || write.Value, // 如果强制要求写入深度，则启用深度测试，否则写不进去
                    Write = (NativeBool)write.Value,
                    Compare = CompareFunction.Always, // 深度测试关闭时，默认通过
                };
            }
            else
            {
                ShaderPassVar<bool> write = m_ZWrite ?? parent.m_ZWrite ?? true;

                return new ShaderPassDepthState
                {
                    Enable = true,
                    Write = (NativeBool)write.Value,
                    Compare = m_ZTest ?? parent.m_ZTest ?? CompareFunction.Less,
                };
            }
        }

        public ShaderPassBlendState[] GetBlendStates(ParsedMetadataContainer parent)
        {
            ShaderPassBlendState[] blends = Enumerable.Repeat(ShaderPassBlendState.Default, 8).ToArray();

            ParsedBlend[]? parsedBlends = m_Blends ?? parent.m_Blends;
            ParsedBlendOp[]? parsedOps = m_BlendOps ?? parent.m_BlendOps;
            ShaderPassVar<ColorWriteMask>[]? writeMasks = m_ColorMasks ?? parent.m_ColorMasks;

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

    internal class ParsedShaderProperty
    {
        public int LineNumber;
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
    }

    internal class ParsedShaderData : ParsedMetadataContainer
    {
        public string Name = "UnnamedShader";
        public List<ParsedShaderProperty> Properties = [];
        public List<string> HlslIncludes = [];
        public List<ParsedShaderPass> Passes = [];

        public string GetSourceCode(int passIndex)
        {
            if (passIndex < 0 || passIndex >= Passes.Count)
            {
                return string.Empty;
            }

            string code = DefineMaterialProperties();

            ParsedShaderPass pass = Passes[passIndex];
            code += "\n" + string.Join("\n", HlslIncludes);

            if (pass.HlslProgram != null)
            {
                code += "\n" + pass.HlslProgram;
            }

            return code;
        }

        private string DefineMaterialProperties()
        {
            using var texBuilder = StringBuilderPool.Get();
            using var cbBuilder = StringBuilderPool.Get();

            foreach (ParsedShaderProperty prop in Properties)
            {
                switch (prop.Type)
                {
                    case ShaderPropertyType.Texture:
                        string type = prop.TexDimension switch
                        {
                            TextureDimension.Tex2D => "Texture2D",
                            TextureDimension.Tex3D => "Texture3D",
                            TextureDimension.Cube => "TextureCube",
                            TextureDimension.Tex2DArray => "Texture2DArray",
                            TextureDimension.CubeArray => "TextureCubeArray",
                            _ => throw new NotSupportedException("Unknown texture dimension"),
                        };

                        // 保证行号和源文件一致
                        // 注意：#line 设置的是下一行的行号
                        texBuilder.Value.AppendLine($"#line {prop.LineNumber}");
                        texBuilder.Value.AppendLine($"{type} {prop.Name}; SamplerState sampler{prop.Name};");
                        break;
                    case ShaderPropertyType.Float:
                        cbBuilder.Value.AppendLine($"#line {prop.LineNumber}");
                        cbBuilder.Value.AppendLine($"float {prop.Name};");
                        break;
                    case ShaderPropertyType.Int:
                        cbBuilder.Value.AppendLine($"#line {prop.LineNumber}");
                        cbBuilder.Value.AppendLine($"int {prop.Name};");
                        break;
                    case ShaderPropertyType.Color:
                    case ShaderPropertyType.Vector:
                        cbBuilder.Value.AppendLine($"#line {prop.LineNumber}");
                        cbBuilder.Value.AppendLine($"float4 {prop.Name};");
                        break;
                    default:
                        throw new NotSupportedException("Unknown property type");
                }
            }

            if (cbBuilder.Value.Length > 0)
            {
                string cbName = ShaderUtility.GetStringFromId(Shader.MaterialConstantBufferId);
                cbBuilder.Value.Insert(0, $"cbuffer {cbName}\n{{\n");
                cbBuilder.Value.AppendLine("};");
            }

            return texBuilder.Value.ToString() + cbBuilder.Value.ToString();
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

            prop.LineNumber = context.Identifier().Symbol.Line;
            prop.Name = context.Identifier().GetText();
            prop.Label = context.StringLiteral().GetText()[1..^1]; // remove quotes

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

            var type = context.propertyTypeDeclaration();
            Visit(type);

            var defaultValue = context.propertyDefaultValueExpression();
            Visit(defaultValue);

            switch (type.GetChild<ITerminalNode>(0).Symbol.Type)
            {
                case ShaderLabParser.Float:
                    prop.Type = ShaderPropertyType.Float;
                    prop.DefaultFloat = float.Parse(defaultValue.numberLiteralExpression().GetText());
                    break;

                case ShaderLabParser.Int:
                    prop.Type = ShaderPropertyType.Int;
                    prop.DefaultInt = int.Parse(defaultValue.numberLiteralExpression().GetText());
                    break;

                case ShaderLabParser.Color:
                    prop.Type = ShaderPropertyType.Color;
                    prop.DefaultColor = ParseColor(defaultValue.vectorLiteralExpression());
                    break;

                case ShaderLabParser.Vector:
                    prop.Type = ShaderPropertyType.Vector;
                    prop.DefaultVector = ParseVector(defaultValue.vectorLiteralExpression());
                    break;

                case ShaderLabParser.TwoD:
                    prop.Type = ShaderPropertyType.Texture;
                    prop.DefaultTex = ParseTextureValue(defaultValue.textureLiteralExpression());
                    prop.TexDimension = TextureDimension.Tex2D;
                    break;

                case ShaderLabParser.ThreeD:
                    prop.Type = ShaderPropertyType.Texture;
                    prop.DefaultTex = ParseTextureValue(defaultValue.textureLiteralExpression());
                    prop.TexDimension = TextureDimension.Tex3D;
                    break;

                case ShaderLabParser.Cube:
                    prop.Type = ShaderPropertyType.Texture;
                    prop.DefaultTex = ParseTextureValue(defaultValue.textureLiteralExpression());
                    prop.TexDimension = TextureDimension.Cube;
                    break;

                case ShaderLabParser.TwoDArray:
                    prop.Type = ShaderPropertyType.Texture;
                    prop.DefaultTex = ParseTextureValue(defaultValue.textureLiteralExpression());
                    prop.TexDimension = TextureDimension.Tex2DArray;
                    break;

                case ShaderLabParser.CubeArray:
                    prop.Type = ShaderPropertyType.Texture;
                    prop.DefaultTex = ParseTextureValue(defaultValue.textureLiteralExpression());
                    prop.TexDimension = TextureDimension.CubeArray;
                    break;

                default:
                    throw new NotSupportedException("Unknown property type");
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
            if (context.cullModeValue() != null)
            {
                CurrentMetadataContainer.SetCull(context.cullModeValue().GetChild<ITerminalNode>(0).Symbol.Type switch
                {
                    ShaderLabParser.Back => CullMode.Back,
                    ShaderLabParser.Front => CullMode.Front,
                    ShaderLabParser.Off => CullMode.Off,
                    _ => throw new NotSupportedException("Unknown cull mode"),
                });
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                CurrentMetadataContainer.SetCull(propertyName);
            }

            return base.VisitCullDeclaration(context);
        }

        public override int VisitZTestDeclaration([NotNull] ShaderLabParser.ZTestDeclarationContext context)
        {
            if (context.Disabled() != null)
            {
                CurrentMetadataContainer.DisableZTest();
            }
            else if (context.compareFuncValue() != null)
            {
                CurrentMetadataContainer.SetZTest(ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0)));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                CurrentMetadataContainer.SetZTest(propertyName);
            }

            return base.VisitZTestDeclaration(context);
        }

        public override int VisitZWriteDeclaration([NotNull] ShaderLabParser.ZWriteDeclarationContext context)
        {
            if (context.Off() != null)
            {
                CurrentMetadataContainer.SetZWrite(false);
            }
            else if (context.On() != null)
            {
                CurrentMetadataContainer.SetZWrite(true);
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                CurrentMetadataContainer.SetZWrite(propertyName);
            }

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
                    blend.SrcRgb = GetBlendMode(context.blendFactorValueOrBracketLiteral(0));
                    blend.DstRgb = GetBlendMode(context.blendFactorValueOrBracketLiteral(1));

                    if (context.blendFactorValueOrBracketLiteral(2) != null && context.blendFactorValueOrBracketLiteral(3) != null)
                    {
                        blend.SrcAlpha = GetBlendMode(context.blendFactorValueOrBracketLiteral(2));
                        blend.DstAlpha = GetBlendMode(context.blendFactorValueOrBracketLiteral(3));
                    }
                    else
                    {
                        blend.SrcAlpha = blend.SrcRgb;
                        blend.DstAlpha = blend.DstRgb;
                    }
                }
            }

            return base.VisitBlendDeclaration(context);

            static ShaderPassVar<BlendMode> GetBlendMode(ShaderLabParser.BlendFactorValueOrBracketLiteralContext context)
            {
                if (context.blendFactorValue() != null)
                {
                    return ParseBlendFactor(context.blendFactorValue().GetChild<ITerminalNode>(0));
                }
                else
                {
                    string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                    return new ShaderPassVar<BlendMode>(propertyName);
                }
            }
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

                blend.OpRgb = GetBlendOp(context.blendOpValueOrBracketLiteral(0));

                if (context.blendOpValueOrBracketLiteral(1) != null)
                {
                    blend.OpAlpha = GetBlendOp(context.blendOpValueOrBracketLiteral(1));
                }
                else
                {
                    blend.OpAlpha = blend.OpRgb;
                }
            }

            return base.VisitBlendOpDeclaration(context);

            static ShaderPassVar<BlendOp> GetBlendOp(ShaderLabParser.BlendOpValueOrBracketLiteralContext context)
            {
                if (context.blendOpValue() != null)
                {
                    return ParseBlendOp(context.blendOpValue().GetChild<ITerminalNode>(0));
                }
                else
                {
                    string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                    return new ShaderPassVar<BlendOp>(propertyName);
                }
            }
        }

        public override int VisitColorMaskInt1Declaration([NotNull] ShaderLabParser.ColorMaskInt1DeclarationContext context)
        {
            if (context.IntegerLiteral().GetText() != "0")
            {
                throw new NotSupportedException("Invalid color write mask");
            }

            ShaderPassVar<ColorWriteMask>[] colorMasks = CurrentMetadataContainer.SetColorMasks();

            for (int i = 0; i < colorMasks.Length; i++)
            {
                colorMasks[i] = ColorWriteMask.None;
            }

            return base.VisitColorMaskInt1Declaration(context);
        }

        public override int VisitColorMaskInt2Declaration([NotNull] ShaderLabParser.ColorMaskInt2DeclarationContext context)
        {
            ShaderPassVar<ColorWriteMask>[] colorMasks = CurrentMetadataContainer.SetColorMasks();

            int i = int.Parse(context.IntegerLiteral(0).GetText());

            if (i < 0 || i >= colorMasks.Length)
            {
                throw new NotSupportedException("Invalid render target index");
            }

            if (context.IntegerLiteral(1).GetText() != "0")
            {
                throw new NotSupportedException("Invalid color write mask");
            }

            colorMasks[i] = ColorWriteMask.None;

            return base.VisitColorMaskInt2Declaration(context);
        }

        public override int VisitColorMaskIdentifierDeclaration([NotNull] ShaderLabParser.ColorMaskIdentifierDeclarationContext context)
        {
            ShaderPassVar<ColorWriteMask>[] colorMasks = CurrentMetadataContainer.SetColorMasks();

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

            ColorWriteMask mask = ColorWriteMask.None;
            HashSet<char> chars = [];

            foreach (char c in context.Identifier().GetText().ToLower())
            {
                if (!chars.Add(c) || (c != 'r' && c != 'g' && c != 'b' && c != 'a'))
                {
                    throw new NotSupportedException("Invalid color write mask");
                }

                switch (c)
                {
                    case 'r': mask |= ColorWriteMask.Red; break;
                    case 'g': mask |= ColorWriteMask.Green; break;
                    case 'b': mask |= ColorWriteMask.Blue; break;
                    case 'a': mask |= ColorWriteMask.Alpha; break;
                }
            }

            for (int i = begin; i < end; i++)
            {
                colorMasks[i] = mask;
            }

            return base.VisitColorMaskIdentifierDeclaration(context);
        }

        public override int VisitColorMaskBracketLiteralDeclaration([NotNull] ShaderLabParser.ColorMaskBracketLiteralDeclarationContext context)
        {
            ShaderPassVar<ColorWriteMask>[] colorMasks = CurrentMetadataContainer.SetColorMasks();

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

            string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号

            for (int i = begin; i < end; i++)
            {
                colorMasks[i] = new ShaderPassVar<ColorWriteMask>(propertyName);
            }

            return base.VisitColorMaskBracketLiteralDeclaration(context);
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

            if (context.IntegerLiteral() != null)
            {
                stencil.Ref = byte.Parse(context.IntegerLiteral().GetText());
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.Ref = new ShaderPassVar<byte>(propertyName);
            }

            return base.VisitStencilRefDeclaration(context);
        }

        public override int VisitStencilReadMaskDeclaration([NotNull] ShaderLabParser.StencilReadMaskDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.IntegerLiteral() != null)
            {
                int numBase = 10;
                string mask = context.IntegerLiteral().GetText();

                if (mask.StartsWith("0x"))
                {
                    numBase = 16;
                    mask = mask[2..];
                }

                stencil.ReadMask = Convert.ToByte(mask, numBase);
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.ReadMask = new ShaderPassVar<byte>(propertyName);
            }

            return base.VisitStencilReadMaskDeclaration(context);
        }

        public override int VisitStencilWriteMaskDeclaration([NotNull] ShaderLabParser.StencilWriteMaskDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.IntegerLiteral() != null)
            {
                int numBase = 10;
                string mask = context.IntegerLiteral().GetText();

                if (mask.StartsWith("0x"))
                {
                    numBase = 16;
                    mask = mask[2..];
                }

                stencil.WriteMask = Convert.ToByte(mask, numBase);
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.WriteMask = new ShaderPassVar<byte>(propertyName);
            }

            return base.VisitStencilWriteMaskDeclaration(context);
        }

        public override int VisitStencilCompDeclaration([NotNull] ShaderLabParser.StencilCompDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.compareFuncValue() != null)
            {
                CompareFunction compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
                stencil.FrontFace.Compare = compare;
                stencil.BackFace.Compare = compare;
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.Compare = new ShaderPassVar<CompareFunction>(propertyName);
                stencil.BackFace.Compare = new ShaderPassVar<CompareFunction>(propertyName);
            }

            return base.VisitStencilCompDeclaration(context);
        }

        public override int VisitStencilPassDeclaration([NotNull] ShaderLabParser.StencilPassDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                StencilOp op = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
                stencil.FrontFace.PassOp = op;
                stencil.BackFace.PassOp = op;
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.PassOp = new ShaderPassVar<StencilOp>(propertyName);
                stencil.BackFace.PassOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilPassDeclaration(context);
        }

        public override int VisitStencilFailDeclaration([NotNull] ShaderLabParser.StencilFailDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                StencilOp op = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
                stencil.FrontFace.FailOp = op;
                stencil.BackFace.FailOp = op;
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.FailOp = new ShaderPassVar<StencilOp>(propertyName);
                stencil.BackFace.FailOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilFailDeclaration(context);
        }

        public override int VisitStencilZFailDeclaration([NotNull] ShaderLabParser.StencilZFailDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                StencilOp op = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
                stencil.FrontFace.DepthFailOp = op;
                stencil.BackFace.DepthFailOp = op;
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.DepthFailOp = new ShaderPassVar<StencilOp>(propertyName);
                stencil.BackFace.DepthFailOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilZFailDeclaration(context);
        }

        public override int VisitStencilCompFrontDeclaration([NotNull] ShaderLabParser.StencilCompFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.compareFuncValue() != null)
            {
                stencil.FrontFace.Compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.Compare = new ShaderPassVar<CompareFunction>(propertyName);
            }

            return base.VisitStencilCompFrontDeclaration(context);
        }

        public override int VisitStencilPassFrontDeclaration([NotNull] ShaderLabParser.StencilPassFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                stencil.FrontFace.PassOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.PassOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilPassFrontDeclaration(context);
        }

        public override int VisitStencilFailFrontDeclaration([NotNull] ShaderLabParser.StencilFailFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                stencil.FrontFace.FailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.FailOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilFailFrontDeclaration(context);
        }

        public override int VisitStencilZFailFrontDeclaration([NotNull] ShaderLabParser.StencilZFailFrontDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                stencil.FrontFace.DepthFailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.FrontFace.DepthFailOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilZFailFrontDeclaration(context);
        }

        public override int VisitStencilCompBackDeclaration([NotNull] ShaderLabParser.StencilCompBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.compareFuncValue() != null)
            {
                stencil.BackFace.Compare = ParseCompareFunc(context.compareFuncValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.BackFace.Compare = new ShaderPassVar<CompareFunction>(propertyName);
            }

            return base.VisitStencilCompBackDeclaration(context);
        }

        public override int VisitStencilPassBackDeclaration([NotNull] ShaderLabParser.StencilPassBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                stencil.BackFace.PassOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.BackFace.PassOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilPassBackDeclaration(context);
        }

        public override int VisitStencilFailBackDeclaration([NotNull] ShaderLabParser.StencilFailBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                stencil.BackFace.FailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.BackFace.FailOp = new ShaderPassVar<StencilOp>(propertyName);
            }

            return base.VisitStencilFailBackDeclaration(context);
        }

        public override int VisitStencilZFailBackDeclaration([NotNull] ShaderLabParser.StencilZFailBackDeclarationContext context)
        {
            ref ShaderPassStencilState stencil = ref CurrentMetadataContainer.SetStencilState();

            if (context.stencilOpValue() != null)
            {
                stencil.BackFace.DepthFailOp = ParseStencilOp(context.stencilOpValue().GetChild<ITerminalNode>(0));
            }
            else
            {
                string propertyName = context.BracketLiteral().GetText()[1..^1]; // 去掉方括号
                stencil.BackFace.DepthFailOp = new ShaderPassVar<StencilOp>(propertyName);
            }

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

        private static DefaultTexture ParseTextureValue(ShaderLabParser.TextureLiteralExpressionContext context)
        {
            string value = context.StringLiteral().GetText()[1..^1]; // Remove quotes

            if (string.IsNullOrEmpty(value))
            {
                return DefaultTexture.Gray;
            }

            return Enum.Parse<DefaultTexture>(value, true); // Ignore case
        }

        private static CompareFunction ParseCompareFunc(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Never => CompareFunction.Never,
                ShaderLabParser.Less => CompareFunction.Less,
                ShaderLabParser.Equal => CompareFunction.Equal,
                ShaderLabParser.LEqual => CompareFunction.LessEqual,
                ShaderLabParser.Greater => CompareFunction.Greater,
                ShaderLabParser.NotEqual => CompareFunction.NotEqual,
                ShaderLabParser.GEqual => CompareFunction.GreaterEqual,
                ShaderLabParser.Always => CompareFunction.Always,
                _ => throw new NotSupportedException("Unknown comparison function"),
            };
        }

        private static BlendMode ParseBlendFactor(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Zero => BlendMode.Zero,
                ShaderLabParser.One => BlendMode.One,
                ShaderLabParser.SrcColor => BlendMode.SrcColor,
                ShaderLabParser.OneMinusSrcColor => BlendMode.InvSrcColor,
                ShaderLabParser.SrcAlpha => BlendMode.SrcAlpha,
                ShaderLabParser.OneMinusSrcAlpha => BlendMode.InvSrcAlpha,
                ShaderLabParser.DstAlpha => BlendMode.DestAlpha,
                ShaderLabParser.OneMinusDstAlpha => BlendMode.InvDestAlpha,
                ShaderLabParser.DstColor => BlendMode.DestColor,
                ShaderLabParser.OneMinusDstColor => BlendMode.InvDestColor,
                ShaderLabParser.SrcAlphaSaturate => BlendMode.SrcAlphaSat,
                _ => throw new NotSupportedException("Unknown blend factor"),
            };
        }

        private static BlendOp ParseBlendOp(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Add => BlendOp.Add,
                ShaderLabParser.Sub => BlendOp.Subtract,
                ShaderLabParser.RevSub => BlendOp.RevSubtract,
                ShaderLabParser.Min => BlendOp.Min,
                ShaderLabParser.Max => BlendOp.Max,
                _ => throw new NotSupportedException("Unknown blend operation"),
            };
        }

        private static StencilOp ParseStencilOp(ITerminalNode node)
        {
            return node.Symbol.Type switch
            {
                ShaderLabParser.Keep => StencilOp.Keep,
                ShaderLabParser.Zero => StencilOp.Zero,
                ShaderLabParser.Replace => StencilOp.Replace,
                ShaderLabParser.IncrSat => StencilOp.IncrSat,
                ShaderLabParser.DecrSat => StencilOp.DecrSat,
                ShaderLabParser.Invert => StencilOp.Invert,
                ShaderLabParser.IncrWrap => StencilOp.Incr,
                ShaderLabParser.DecrWrap => StencilOp.Decr,
                _ => throw new NotSupportedException("Unknown stencil operation"),
            };
        }

        [GeneratedRegex(@"^(.+?)(\((.*)\))?$")]
        private static partial Regex AttributeRegex();
    }
}
