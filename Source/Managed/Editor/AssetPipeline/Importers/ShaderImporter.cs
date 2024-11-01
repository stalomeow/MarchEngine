using Antlr4.Runtime;
using March.Core;
using March.Core.IconFont;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.ShaderLab;
using March.Editor.ShaderLab.Internal;
using Newtonsoft.Json;
using System.Collections.Immutable;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Shader Asset", ".shader", Version = 47)]
    internal class ShaderImporter : AssetImporter
    {
        [JsonProperty]
        [HideInInspector]
        private bool m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;

        [JsonProperty]
        [HideInInspector]
        private GraphicsColorSpace m_ColorSpace = GraphicsSettings.ColorSpace;

        protected override bool NeedReimport
        {
            get => m_UseReversedZBuffer != GraphicsSettings.UseReversedZBuffer
                || m_ColorSpace != GraphicsSettings.ColorSpace
                || base.NeedReimport;
        }

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;
            m_ColorSpace = GraphicsSettings.ColorSpace;

            Shader shader = context.AddMainAsset<Shader>(normalIcon: FontAwesome6.Code);
            CompileShader(shader, File.ReadAllText(Location.AssetFullPath, Encoding.UTF8));
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
                CompileShaderPassWithFallback(shader, i, result.GetSourceCode(i));
            }
        }

        private void CompileShaderPassWithFallback(Shader shader, int passIndex, string source)
        {
            if (shader.CompilePass(passIndex, Location.AssetFullPath, source))
            {
                return;
            }

            if (!shader.CompilePass(passIndex, Location.AssetFullPath, FallbackShaderCodes.Program))
            {
                Debug.LogError($"Failed to compile fallback shader program");
            }
        }

        private ParsedShaderData ParseShaderLabWithFallback(Shader shader, string code)
        {
            ParsedShaderData result = ParseShaderLab(Location.AssetFullPath, code, out ImmutableArray<string> errors);

            if (!errors.IsEmpty)
            {
                foreach (string e in errors)
                {
                    if (shader.AddError(e))
                    {
                        Debug.LogError(e);
                    }
                }

                result = ParseShaderLab(Location.AssetFullPath, FallbackShaderCodes.ShaderLab, out errors);
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
            public const string Program = @"
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
";

            public const string ShaderLab = $@"
Shader ""ErrorShader""
{{
    Pass
    {{
        Name ""ErrorPass""

        Cull Off

        HLSLPROGRAM
        {Program}
        ENDHLSL
    }}
}}";
        }
    }

    internal class ShaderImporterDrawer : AssetImporterDrawerFor<ShaderImporter>
    {
        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = base.DrawProperties(out showApplyRevertButtons);

            Shader shader = (Shader)Target.MainAsset;

            EditorGUI.LabelField("Warnings", string.Empty, shader.Warnings.Length.ToString());
            EditorGUI.LabelField("Errors", string.Empty, shader.Errors.Length.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                foreach (string warning in shader.Warnings)
                {
                    Debug.LogWarning(warning);
                }

                foreach (string error in shader.Errors)
                {
                    Debug.LogError(error);
                }
            }

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Properties", string.Empty, defaultOpen: true))
            {
                using (new EditorGUI.IndentedScope())
                {
                    foreach (ShaderProperty prop in shader.Properties)
                    {
                        EditorGUI.LabelField(prop.Name, string.Empty, prop.Type.ToString());
                    }
                }
            }

            return isChanged;
        }
    }
}
