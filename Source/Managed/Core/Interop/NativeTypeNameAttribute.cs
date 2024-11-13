namespace March.Core.Interop
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, AllowMultiple = false, Inherited = false)]
    public sealed class NativeTypeNameAttribute(string name) : Attribute
    {
        public string Name => name;
    }
}
