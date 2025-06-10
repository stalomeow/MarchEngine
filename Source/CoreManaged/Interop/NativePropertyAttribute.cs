namespace March.Core.Interop
{
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false, Inherited = false)]
    public sealed class NativePropertyAttribute : Attribute
    {
        public string? Name { get; }

        public string? This { get; init; }

        public NativePropertyAttribute() { }

        public NativePropertyAttribute(string name) => Name = name;
    }
}
