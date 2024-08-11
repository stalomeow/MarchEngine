using DX12Demo.Core;
using Newtonsoft.Json;

namespace DX12Demo.Editor
{
    public abstract class AssetImporter : EngineObject
    {
        [JsonIgnore]
        public abstract string DisplayName { get; }

        [JsonIgnore]
        public string AssetPath { get; internal set; } = string.Empty;

        public abstract EngineObject Import(string path);
    }

    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false)]
    public sealed class CustomAssetImporterAttribute(params string[] extensionsIncludingLeadingDot) : Attribute
    {
        /// <summary>
        /// 扩展名，前面有 '.'
        /// </summary>
        public string[] Extensions { get; } = extensionsIncludingLeadingDot;
    }
}
