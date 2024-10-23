using Antlr4.Runtime;
using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Core.Serialization;
using March.Editor.ShaderLab;
using March.Editor.ShaderLab.Internal;
using Newtonsoft.Json;
using System.Text;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".shader")]
    internal class ShaderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => base.Version + 28;

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
            string shaderCode = File.ReadAllText(AssetFullPath, Encoding.UTF8);

            try
            {
                CompileShader(asset, shaderCode, clearWarningsAndErrors: true);
            }
            catch
            {
                CompileShader(asset, s_ErrorShader, clearWarningsAndErrors: false);
            }
        }

        private void CompileShader(MarchObject asset, string shaderCode, bool clearWarningsAndErrors)
        {
            AntlrInputStream inputStream = new(shaderCode);
            ShaderLabLexer lexer = new(inputStream);
            CommonTokenStream tokenStream = new(lexer);
            ShaderLabParser parser = new(tokenStream);

            ShaderLabVisitor visitor = new();
            visitor.Visit(parser.shader());
            ParsedShaderData result = visitor.Data;

            Shader shader = (Shader)asset;
            shader.Name = result.Name ?? throw new NotSupportedException("Shader name is required.");
            shader.Properties = result.Properties.Select(p => new ShaderProperty
            {
                Name = p.Name ?? throw new NotSupportedException("Shader property name is required."),
                Label = p.Label ?? throw new NotSupportedException("Shader property label is required."),
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
                Cull = p.GetCull(result),
                Blends = p.GetBlendStates(result),
                DepthState = p.GetDepthState(result),
                StencilState = p.GetStencilState(result),
            }).ToArray();

            shader.UploadPropertyDataToNative();

            if (clearWarningsAndErrors)
            {
                shader.ClearWarningsAndErrors();
            }

            for (int i = 0; i < result.Passes.Count; i++)
            {
                string code = result.GetShaderProgramCode(i, out string shaderModel,
                    out Dictionary<ShaderProgramType, string> entrypoints);

                if (entrypoints.TryGetValue(ShaderProgramType.Vertex, out string? vs))
                {
                    if (!shader.CompilePass(i, AssetFullPath, code, vs, shaderModel, ShaderProgramType.Vertex))
                    {
                        throw new Exception();
                    }
                }

                if (entrypoints.TryGetValue(ShaderProgramType.Pixel, out string? ps))
                {
                    if (!shader.CompilePass(i, AssetFullPath, code, ps, shaderModel, ShaderProgramType.Pixel))
                    {
                        throw new Exception();
                    }
                }

                shader.CreatePassRootSignature(i);
            }
        }

        private const string s_ErrorShader = @"
Shader ""ErrorShader""
{
    Pass
    {
        Name ""ErrorShaderPass""

        Cull Off
        ZTest Less
        ZWrite On

        Blend 0 Off

        HLSLPROGRAM
        #pragma target 6.0
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
    }
}
