namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class ColorDrawerAttribute : Attribute
    {
        public bool Alpha { get; set; } = true;

        public bool HDR { get; set; } = false;
    }
}
