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

    [CustomAssetImporter(".shader")]
    internal class ShaderImporter : AssetImporter
    {
        public override string DisplayName => "Shader Asset";

        protected override int Version => 4;

        protected override bool UseCache => true;

        protected override EngineObject CreateAsset(bool willSaveToFile)
        {
            string json = File.ReadAllText(AssetFullPath, Encoding.UTF8);
            var rawData = JsonConvert.DeserializeObject<ShaderRawData>(json, s_JsonSettings);


        }

        private static readonly JsonSerializerSettings s_JsonSettings = new()
        {
            ObjectCreationHandling = ObjectCreationHandling.Replace, // 保证不会重新添加集合元素
        };
    }
}
