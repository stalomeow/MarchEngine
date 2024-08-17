using DX12Demo.Core;
using DX12Demo.Core.Rendering;
using Newtonsoft.Json;

namespace DX12Demo.Editor.Importers
{
    internal class ShaderIncludeImportData(string target)
    {
        public string Filename = "Include.hlsl";
        public string Target = target;

        public ShaderIncludeImportData() : this("vs_5_0") { }
    }

    internal class ShaderPassImportData
    {
        public string Name = "PassName";
        public ShaderIncludeImportData VertexShader = new("vs_5_0");
        public ShaderIncludeImportData PixelShader = new("ps_5_0");
        public ShaderPassCullMode Cull = ShaderPassCullMode.Back;
        public List<ShaderPassBlendState> Blends = [new ShaderPassBlendState()];
        public ShaderPassDepthStencilState DepthStencilState = new();
    }

    [CustomAssetImporter(".shader")]
    internal class ShaderImporter : AssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => 2;

        protected override bool UseCache => true;

        [JsonProperty]
        private string m_ShaderName = "New Shader";

        [JsonProperty]
        private List<ShaderProperty> m_ShaderProperties = new();

        [JsonProperty]
        private List<ShaderPassImportData> m_ShaderPasses = new();

        protected override EngineObject CreateAsset(bool willSaveToFile)
        {
            var shader = new Shader();

            foreach (ShaderProperty prop in m_ShaderProperties)
            {
                shader.Properties.Add(prop with { });
            }

            return shader;
        }
    }
}
