namespace March.Editor.AssetPipeline
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public sealed class CustomAssetImporterAttribute(string displayName, params string[] extensionsIncludingLeadingDot) : Attribute
    {
        /// <summary>
        /// Inspector 上展示的名称
        /// </summary>
        public string DisplayName { get; } = displayName;

        /// <summary>
        /// 支持的扩展名，前面有 '.'
        /// </summary>
        public string[] Extensions { get; } = extensionsIncludingLeadingDot;

        /// <summary>
        /// 代码的版本，每次修改代码时都必须增加这个版本号
        /// </summary>
        public int Version { get; set; }
    }
}
