namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class TooltipAttribute(string tooltip) : Attribute
    {
        public string Tooltip { get; } = tooltip;
    }
}
