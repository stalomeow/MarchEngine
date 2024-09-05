using Antlr4.Runtime;
using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using DX12Demo.Editor.ShaderLab;
using DX12Demo.Editor.ShaderLab.Internal;
using System.Text;

namespace DX12Demo.Editor.Importers
{
    [CustomAssetImporter(".shader")]
    internal class ShaderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => 11;

        protected override bool UseCache => true;

        protected override EngineObject CreateAsset()
        {
            return new Shader();
        }

        protected override void PopulateAsset(EngineObject asset, bool willSaveToFile)
        {
            string shaderCode = File.ReadAllText(AssetFullPath, Encoding.UTF8);

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
                    shader.CompilePass(i, AssetFullPath, pass.HlslProgram, pass.VsEntrypoint, pass.ShaderModel, ShaderProgramType.Vertex);
                }

                if (pass.PsEntrypoint != null)
                {
                    shader.CompilePass(i, AssetFullPath, pass.HlslProgram, pass.PsEntrypoint, pass.ShaderModel, ShaderProgramType.Pixel);
                }

                shader.CreatePassRootSignature(i);
            }
        }
    }
}
