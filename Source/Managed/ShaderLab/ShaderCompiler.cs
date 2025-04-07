using Antlr4.Runtime;
using March.Core.Diagnostics;
using March.Core.Rendering;
using March.ShaderLab.Internal;
using System.Collections.Immutable;

namespace March.ShaderLab
{
    public static class ShaderCompiler
    {
        public static void CompileShaderLab(Shader shader, string fullPath, string content, List<string> outPassSourceCodes, List<string> outErrors)
        {
            ParsedShaderData result = ParseShaderLabWithFallback(fullPath, content, outErrors);

            shader.Name = result.Name;
            shader.Properties = result.Properties.Select(p => new ShaderProperty()
            {
                Name = p.Name,
                Label = p.Label,
                Tooltip = p.Tooltip,
                Attributes = p.Attributes,
                Type = p.Type,
                DefaultFloat = p.DefaultFloat,
                DefaultInt = p.DefaultInt,
                DefaultColor = p.DefaultColor,
                DefaultVector = p.DefaultVector,
                TexDimension = p.TexDimension,
                DefaultTex = p.DefaultTex,
            }).ToArray();
            shader.Passes = result.Passes.Select(p => new ShaderPass()
            {
                Name = p.Name,
                Tags = p.GetTags(result),
                Cull = p.GetCull(result),
                Blends = p.GetBlendStates(result),
                DepthState = p.GetDepthState(result),
                StencilState = p.GetStencilState(result),
            }).ToArray();

            for (int i = 0; i < result.Passes.Count; i++)
            {
                outPassSourceCodes.Add(result.GetSourceCode(i));
            }
        }

        private static ParsedShaderData ParseShaderLabWithFallback(string fullPath, string content, List<string> outErrors)
        {
            ParsedShaderData result = ParseShaderLab(fullPath, content, out ImmutableArray<string> errors);

            if (!errors.IsEmpty)
            {
                outErrors.AddRange(errors);
                result = ParseShaderLab(fullPath, FallbackShaderLab, out errors);

                if (!errors.IsEmpty)
                {
                    Log.Message(LogLevel.Error, "Failed to parse fallback ShaderLab");
                }
            }

            return result;
        }

        private static ParsedShaderData ParseShaderLab(string fullPath, string content, out ImmutableArray<string> errors)
        {
            var errorListener = new ShaderLabErrorListener(fullPath);

            var inputStream = new AntlrInputStream(content);
            var lexer = new ShaderLabLexer(inputStream);
            lexer.RemoveErrorListeners();
            lexer.AddErrorListener(errorListener);

            // 默认的 DefaultErrorStrategy 遇到错误时会调用 ErrorListener，然后尝试恢复错误
            // BailErrorStrategy 会在遇到错误时立即停止解析，不会调用 ErrorListener，不方便收集错误信息

            var tokenStream = new CommonTokenStream(lexer);
            var parser = new ShaderLabParser(tokenStream);
            parser.RemoveErrorListeners();
            parser.AddErrorListener(errorListener);

            var visitor = new ShaderLabVisitor(errorListener);
            visitor.Visit(parser.shader());

            errors = errorListener.Errors;
            return visitor.Data;
        }

        public static void GetHLSLIncludesAndPragmas(string hlsl, out List<string> includes, out List<string> pragmas)
        {
            try
            {
                var inputStream = new AntlrInputStream(hlsl);
                var lexer = new HLSLPreprocessorLexer(inputStream);

                var tokenStream = new CommonTokenStream(lexer);
                var parser = new HLSLPreprocessorParser(tokenStream);

                var visitor = new HLSLPreprocessorVisitor();
                visitor.Visit(parser.program());

                includes = visitor.Includes;
                pragmas = visitor.Pragmas;
            }
            catch (Exception e)
            {
                Log.Message(LogLevel.Error, "Failed to parse HLSL includes and pragmas", $"{e}");

                includes = [];
                pragmas = [];
            }
        }

        public const string FallbackProgram = @"
#pragma vs vert
#pragma ps frag

#include ""Includes/Common.hlsl""

float4 vert(float3 positionOS : POSITION, uint instanceID : SV_InstanceID) : SV_Position
{
    float3 positionWS = TransformObjectToWorld(instanceID, positionOS);
    return TransformWorldToHClip(positionWS);
}

float4 frag() : SV_Target
{
    return float4(1, 0, 1, 1);
}
";

        public const string FallbackShaderLab = $@"
Shader ""ErrorShader""
{{
    Pass
    {{
        Name ""ErrorPass""

        Cull Off

        HLSLPROGRAM
        {FallbackProgram}
        ENDHLSL
    }}
}}";
    }
}
