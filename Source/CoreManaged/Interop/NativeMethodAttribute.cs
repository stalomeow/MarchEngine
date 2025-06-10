namespace March.Core.Interop
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class NativeMethodAttribute : Attribute
    {
        public string? Name { get; }

        public string? This { get; init; }

        public NativeMethodAttribute() { }

        public NativeMethodAttribute(string name) => Name = name;
    }
}
