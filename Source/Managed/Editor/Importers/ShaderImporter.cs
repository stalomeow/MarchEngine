using Antlr4.Runtime;
using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.ShaderLab;
using March.Editor.ShaderLab.Internal;
using Newtonsoft.Json;
using System.Collections.Immutable;
using System.Text;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".shader")]
    internal class ShaderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => base.Version + 45;

        public override string IconNormal => FontAwesome6.Code;

        protected override bool UseCache => true;

        [JsonProperty]
        [HideInInspector]
        private bool m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;

        [JsonProperty]
        [HideInInspector]
        private GraphicsColorSpace m_ColorSpace = GraphicsSettings.ColorSpace;

        public override bool NeedReimportAsset()
        {
            return (m_UseReversedZBuffer != GraphicsSettings.UseReversedZBuffer)
                || (m_ColorSpace != GraphicsSettings.ColorSpace)
                || base.NeedReimportAsset();
        }

        protected override void OnWillSaveImporter()
        {
            base.OnWillSaveImporter();
            m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;
            m_ColorSpace = GraphicsSettings.ColorSpace;
        }

        protected override MarchObject CreateAsset()
        {
            return new Shader();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            CompileShader((Shader)asset, File.ReadAllText(AssetFullPath, Encoding.UTF8));
        }

        private void CompileShader(Shader shader, string content)
        {
            shader.ClearWarningsAndErrors();
            ParsedShaderData result = ParseShaderLabWithFallback(shader, content);

            shader.Name = result.Name!;
            shader.Properties = result.Properties.Select(p => new ShaderProperty
            {
                Name = p.Name!,
                Label = p.Label!,
                Tooltip = p.Tooltip,
                Attributes = p.Attributes.ToArray(),
                Type = p.Type,
                DefaultFloat = p.DefaultFloat,
                DefaultInt = p.DefaultInt,
                DefaultColor = p.DefaultColor,
                DefaultVector = p.DefaultVector,
                DefaultTexture = p.DefaultTexture
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
                string program = result.GetShaderProgramCode(i, out string shaderModel, out bool enableDebugInfo,
                    out Dictionary<ShaderProgramType, string> entrypoints);

                if (entrypoints.TryGetValue(ShaderProgramType.Vertex, out string? vsEntrypoint))
                {
                    CompileShaderPassWithFallback(shader, i, program, vsEntrypoint, shaderModel, enableDebugInfo, ShaderProgramType.Vertex);
                }

                if (entrypoints.TryGetValue(ShaderProgramType.Pixel, out string? psEntrypoint))
                {
                    CompileShaderPassWithFallback(shader, i, program, psEntrypoint, shaderModel, enableDebugInfo, ShaderProgramType.Pixel);
                }
            }

            shader.CreateRootSignatures();
        }

        private void CompileShaderPassWithFallback(Shader shader, int passIndex, string program, string entrypoint, string shaderModel, bool enableDebugInfo, ShaderProgramType type)
        {
            if (shader.CompilePass(passIndex, AssetFullPath, program, entrypoint, shaderModel, type, enableDebugInfo))
            {
                return;
            }

            switch (type)
            {
                case ShaderProgramType.Vertex:
                    program = FallbackShaderCodes.VertexShaderProgram;
                    entrypoint = FallbackShaderCodes.VertexShaderEntrypoint;
                    break;

                case ShaderProgramType.Pixel:
                    program = FallbackShaderCodes.PixelShaderProgram;
                    entrypoint = FallbackShaderCodes.PixelShaderEntrypoint;
                    break;

                default:
                    Debug.LogError($"Failed to compile shader pass {passIndex} with unknown type {type}");
                    return;
            }

            if (!shader.CompilePass(passIndex, AssetFullPath, program, entrypoint, shaderModel, type, enableDebugInfo))
            {
                Debug.LogError($"Failed to compile shader pass {passIndex} with fallback {type}");
            }
        }

        private ParsedShaderData ParseShaderLabWithFallback(Shader shader, string code)
        {
            ParsedShaderData result = ParseShaderLab(AssetFullPath, code, out ImmutableArray<string> errors);

            if (!errors.IsEmpty)
            {
                foreach (string e in errors)
                {
                    if (shader.AddError(e))
                    {
                        Debug.LogError(e);
                    }
                }

                result = ParseShaderLab(AssetFullPath, FallbackShaderCodes.ErrorShaderLab, out errors);
                Debug.Assert(errors.IsEmpty, "Failed to parse fallback ShaderLab");
            }

            return result;
        }

        private static ParsedShaderData ParseShaderLab(string file, string code, out ImmutableArray<string> errors)
        {
            var errorListener = new ShaderLabErrorListener(file);

            var inputStream = new AntlrInputStream(code);
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

        private static class FallbackShaderCodes
        {
            public const string ErrorShaderLab = @"
Shader ""ErrorShader""
{
    Pass
    {
        Name ""ErrorPass""

        Cull Off

        HLSLPROGRAM
        #pragma vs vert
        #pragma ps frag

        #include ""Includes/Common.hlsl""

        float4 vert(float3 positionOS : POSITION) : SV_Position
        {
            float3 positionWS = TransformObjectToWorld(positionOS);
            return TransformWorldToHClip(positionWS);
        }

        float4 frag() : SV_Target
        {
            return float4(1, 0, 1, 1);
        }
        ENDHLSL
    }
}";

            public const string VertexShaderEntrypoint = "vert";
            public const string VertexShaderProgram = @"
#include ""Includes/Common.hlsl""

float4 vert(float3 positionOS : POSITION) : SV_Position
{
    float3 positionWS = TransformObjectToWorld(positionOS);
    return TransformWorldToHClip(positionWS);
}
";

            public const string PixelShaderEntrypoint = "frag";
            public const string PixelShaderProgram = @"
float4 frag() : SV_Target
{
    return float4(1, 0, 1, 1);
}
";
        }
    }
}
