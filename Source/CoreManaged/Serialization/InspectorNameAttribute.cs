namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class InspectorNameAttribute(string name) : Attribute
    {
        public string Name { get; } = name;
    }
}
