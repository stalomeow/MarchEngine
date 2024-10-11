using Antlr4.Runtime;
using March.Core;
using March.Core.IconFonts;
using March.Core.Rendering;
using March.Editor.ShaderLab;
using March.Editor.ShaderLab.Internal;
using System.Text;

namespace March.Editor.Importers
{
    [CustomAssetImporter(".shader")]
    internal class ShaderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => base.Version + 17;

        public override string IconNormal => FontAwesome6.Code;

        protected override bool UseCache => true;

        protected override MarchObject CreateAsset()
        {
            return new Shader();
        }

        protected override void PopulateAsset(MarchObject asset, bool willSaveToFile)
        {
            string shaderCode = File.ReadAllText(AssetFullPath, Encoding.UTF8);

            try
            {
                CompileShader(asset, shaderCode);
            }
            catch
            {
                CompileShader(asset, s_ErrorShader);
            }
        }

        private void CompileShader(MarchObject asset, string shaderCode)
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
            shader.Passes = result.Passes.Select(p =>
            {
                var pass = new ShaderPass()
                {
                    Name = p.Name,
                    Cull = p.Cull,
                    StencilState = p.Stencil
                };

                if (p.ZTest == null)
                {
                    pass.DepthState = new ShaderPassDepthState
                    {
                        Enable = p.ZWrite,
                        Compare = ShaderPassCompareFunc.Always,
                        Write = p.ZWrite,
                    };
                }
                else
                {
                    pass.DepthState = new ShaderPassDepthState
                    {
                        Enable = true,
                        Compare = p.ZTest.Value,
                        Write = p.ZWrite,
                    };
                }

                int blendCount = 1;

                if (p.Blends.Count > 0)
                {
                    blendCount = p.Blends.Keys.Max() + 1;
                }

                var blends = new ShaderPassBlendState[blendCount];

                for (int i = 0; i < blends.Length; i++)
                {
                    if (p.Blends.TryGetValue(i, out blends[i]))
                    {
                        if (blends[i].WriteMask != ShaderPassColorWriteMask.All)
                        {
                            blends[i].Enable = true;
                        }
                    }
                    else
                    {
                        blends[i] = ShaderPassBlendState.Default;
                    }
                }

                pass.Blends = blends;
                return pass;
            }).ToArray();

            shader.UploadPropertyDataToNative();

            for (int i = 0; i < result.Passes.Count; i++)
            {
                ParsedShaderPass pass = result.Passes[i];

                if (pass.HlslProgram == null)
                {
                    throw new NotSupportedException("Shader pass HLSL program is required.");
                }

                if (pass.VsEntrypoint != null)
                {
                    bool success = shader.CompilePass(i, AssetFullPath, pass.HlslProgram, pass.VsEntrypoint, pass.ShaderModel, ShaderProgramType.Vertex);

                    if (!success)
                    {
                        throw new Exception();
                    }
                }

                if (pass.PsEntrypoint != null)
                {
                    bool success = shader.CompilePass(i, AssetFullPath, pass.HlslProgram, pass.PsEntrypoint, pass.ShaderModel, ShaderProgramType.Pixel);

                    if (!success)
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

        #include ""Lighting.hlsl""

        cbuffer cbObject
        {
            float4x4 _MatrixWorld;
        };

        cbuffer cbPass
        {
            float4x4 _MatrixView;
            float4x4 _MatrixProjection;
            float4x4 _MatrixViewProjection;
            float4x4 _MatrixInvView;
            float4x4 _MatrixInvProjection;
            float4x4 _MatrixInvViewProjection;
            float4 _Time;
            float4 _CameraPositionWS;

            LightData _LightData[MAX_LIGHT_COUNT];
            int _LightCount;
        };

        struct Attributes
        {
            float3 positionOS : POSITION;
        };

        struct Varyings
        {
            float4 positionCS : SV_Position;
        };

        Varyings vert(Attributes input)
        {
            Varyings output;
            float4 positionWS = mul(_MatrixWorld, float4(input.positionOS, 1.0));
            output.positionCS = mul(_MatrixViewProjection, positionWS);
            return output;
        }

        float4 frag(Varyings input) : SV_Target
        {
            return float4(1, 0, 1, 1);
        }
        ENDHLSL
    }
}";
    }
}
