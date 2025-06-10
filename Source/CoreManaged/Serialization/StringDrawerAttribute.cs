namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class StringDrawerAttribute : Attribute
    {
        public string CharBlacklist { get; set; } = "";
    }
}
