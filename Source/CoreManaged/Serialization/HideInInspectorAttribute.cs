namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class HideInInspectorAttribute : Attribute { }
}
