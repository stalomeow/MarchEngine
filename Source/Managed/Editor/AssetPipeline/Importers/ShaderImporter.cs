using Antlr4.Runtime;
using March.Core.Diagnostics;
using March.Core.IconFont;
using March.Core.Pool;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.ShaderLab;
using March.Editor.ShaderLab.Internal;
using Newtonsoft.Json;
using System.Collections.Immutable;
using System.Text;

namespace March.Editor.AssetPipeline.Importers
{
    [CustomAssetImporter("Shader Asset", ".shader", Version = 77)]
    public class ShaderImporter : AssetImporter
    {
        [JsonProperty]
        [HideInInspector]
        private bool m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;

        [JsonProperty]
        [HideInInspector]
        private GraphicsColorSpace m_ColorSpace = GraphicsSettings.ColorSpace;

        [JsonProperty]
        [HideInInspector]
        private List<string> m_Warnings = [];

        [JsonProperty]
        [HideInInspector]
        private List<string> m_Errors = [];

        protected override bool CheckNeedReimport(bool fullCheck)
        {
            return m_UseReversedZBuffer != GraphicsSettings.UseReversedZBuffer
                || m_ColorSpace != GraphicsSettings.ColorSpace
                || base.CheckNeedReimport(fullCheck);
        }

        protected override void OnImportAssets(ref AssetImportContext context)
        {
            m_UseReversedZBuffer = GraphicsSettings.UseReversedZBuffer;
            m_ColorSpace = GraphicsSettings.ColorSpace;

            Shader shader = context.AddMainAsset<Shader>(normalIcon: FontAwesome6Brands.StripeS);
            CompileShader(ref context, shader, File.ReadAllText(Location.AssetFullPath, Encoding.UTF8));
        }

        private void CompileShader(ref AssetImportContext context, Shader shader, string content)
        {
            m_Warnings.Clear();
            m_Errors.Clear();

            ParsedShaderData result = ParseShaderLabWithFallback(shader, content);

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
                CompileShaderPassWithFallback(ref context, shader, i, result.GetSourceCode(i));
            }

            if (m_Warnings.Count > 0)
            {
                m_Warnings = m_Warnings.Distinct().ToList();
            }

            if (m_Errors.Count > 0)
            {
                m_Errors = m_Errors.Distinct().ToList();
            }
        }

        public int WarningCount => m_Warnings.Count;

        public int ErrorCount => m_Errors.Count;

        public override void LogImportMessages()
        {
            base.LogImportMessages();

            foreach (string warning in m_Warnings)
            {
                Log.Message(LogLevel.Warning, warning);
            }

            foreach (string error in m_Errors)
            {
                Log.Message(LogLevel.Error, error);
            }
        }

        private void CompileShaderPassWithFallback(ref AssetImportContext context, Shader shader, int passIndex, string source)
        {
            AddDependency(ref context, source);

            if (shader.CompilePass(passIndex, Location.AssetFullPath, source, m_Warnings, m_Errors))
            {
                return;
            }

            if (shader.CompilePass(passIndex, Location.AssetFullPath, FallbackShaderCodes.Program, m_Warnings, m_Errors))
            {
                // 如果使用了 fallback shader，需要再额外添加 fallback 的依赖
                AddDependency(ref context, FallbackShaderCodes.Program);
            }
            else
            {
                Log.Message(LogLevel.Error, "Failed to compile fallback shader program");
            }
        }

        private void AddDependency(ref AssetImportContext context, string source)
        {
            using var includes = ListPool<AssetLocation>.Get();
            ShaderProgramUtility.GetIncludeFiles(Location.AssetFullPath, source, includes);

            foreach (AssetLocation location in includes.Value)
            {
                context.RequireOtherAsset<ShaderIncludeAsset>(location.AssetPath, dependsOn: true);
            }
        }

        private ParsedShaderData ParseShaderLabWithFallback(Shader shader, string code)
        {
            ParsedShaderData result = ParseShaderLab(Location.AssetFullPath, code, out ImmutableArray<string> errors);

            if (!errors.IsEmpty)
            {
                m_Errors.AddRange(errors);

                result = ParseShaderLab(Location.AssetFullPath, FallbackShaderCodes.ShaderLab, out errors);

                if (!errors.IsEmpty)
                {
                    Log.Message(LogLevel.Error, "Failed to parse fallback ShaderLab");
                }
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
        protected override bool RequireAssets => true;

        protected override bool DrawProperties(out bool showApplyRevertButtons)
        {
            bool isChanged = base.DrawProperties(out showApplyRevertButtons);

            EditorGUI.LabelField("Warnings", string.Empty, Target.WarningCount.ToString());
            EditorGUI.LabelField("Errors", string.Empty, Target.ErrorCount.ToString());

            if (EditorGUI.ButtonRight("Show Warnings and Errors"))
            {
                Target.LogImportMessages();
            }

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Properties", string.Empty, defaultOpen: true))
            {
                Shader shader = (Shader)Target.MainAsset;

                using (new EditorGUI.IndentedScope())
                {
                    foreach (ShaderProperty prop in shader.Properties)
                    {
                        if (prop.Type == ShaderPropertyType.Texture)
                        {
                            using var label = StringBuilderPool.Get();
                            label.Value.Append(prop.Type.ToString()).Append(' ');
                            label.Value.Append('(').Append(prop.TexDimension.ToInspectorEnumName()).Append(')');
                            EditorGUI.LabelField(prop.Name, string.Empty, label);
                        }
                        else
                        {
                            EditorGUI.LabelField(prop.Name, string.Empty, prop.Type.ToString());
                        }
                    }
                }
            }

            EditorGUI.Space();
            EditorGUI.Separator();
            EditorGUI.Space();

            if (EditorGUI.Foldout("Passes", string.Empty, defaultOpen: true))
            {
                Shader shader = (Shader)Target.MainAsset;

                using (new EditorGUI.IndentedScope())
                {
                    foreach (ShaderPass pass in shader.Passes)
                    {
                        EditorGUI.BulletLabel(pass.Name, "");
                    }
                }
            }

            return isChanged;
        }
    }
}
