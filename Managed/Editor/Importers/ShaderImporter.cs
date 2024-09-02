using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using Newtonsoft.Json;
using System.Text;

namespace DX12Demo.Editor.Importers
{
    [JsonObject(NamingStrategyType = typeof(ShaderJsonNamingStrategy))]
    internal class ShaderCodeRawData
    {
        public string Filename = string.Empty;
        public string Entrypoint = string.Empty;
    }

    [JsonObject(NamingStrategyType = typeof(ShaderJsonNamingStrategy))]
    internal class ShaderPassRawData
    {
        public string Name = "<Unnamed Pass>";
        public string ShaderModel = "6.0";
        public ShaderCodeRawData Vs = new();
        public ShaderCodeRawData Ps = new();
        public ShaderPassCullMode Cull = ShaderPassCullMode.Back;
        public List<ShaderPassBlendState> Blends = [new ShaderPassBlendState()];
        public ShaderPassDepthState Depth = new();
        public ShaderPassStencilState Stencil = new();
    }

    [JsonObject(NamingStrategyType = typeof(ShaderJsonNamingStrategy))]
    internal class ShaderRawData
    {
        public string Name = string.Empty;
        public List<ShaderProperty> Properties = [];
        public List<ShaderPassRawData> Passes = [];
    }

    [CustomAssetImporter(".jsonshader")]
    internal class ShaderImporter : ExternalAssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => 5;

        protected override bool UseCache => true;

        protected override EngineObject CreateAsset()
        {
            return new Shader();
        }

        protected override void PopulateAsset(EngineObject asset, bool willSaveToFile)
        {
            string json = File.ReadAllText(AssetFullPath, Encoding.UTF8);
            ShaderRawData? rawData = JsonConvert.DeserializeObject<ShaderRawData>(json, s_JsonSettings);

            if (rawData == null)
            {
                throw new InvalidOperationException("Failed to deserialize shader asset.");
            }

            Shader shader = (Shader)asset;
            shader.Name = rawData.Name;
            shader.Properties = rawData.Properties.ToArray();
            shader.Passes = rawData.Passes.Select(pass => new ShaderPass()
            {
                Name = pass.Name,
                Cull = pass.Cull,
                Blends = pass.Blends.ToArray(),
                DepthState = pass.Depth,
                StencilState = pass.Stencil
            }).ToArray();

            for (int i = 0; i < rawData.Passes.Count; i++)
            {
                ShaderPassRawData pass = rawData.Passes[i];
                shader.CompilePass(i, pass.Vs.Filename, pass.Vs.Entrypoint, pass.ShaderModel, ShaderProgramType.Vertex);
                shader.CompilePass(i, pass.Ps.Filename, pass.Ps.Entrypoint, pass.ShaderModel, ShaderProgramType.Pixel);
            }
        }

        private static readonly JsonSerializerSettings s_JsonSettings = new()
        {
            ObjectCreationHandling = ObjectCreationHandling.Replace, // 保证不会重新添加集合元素
        };
    }
}
