namespace DX12Demo.Core.Binding
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = false)]
    public sealed class NativeFunctionAttribute : Attribute
    {
        public string? Name { get; set; }
    }
}
