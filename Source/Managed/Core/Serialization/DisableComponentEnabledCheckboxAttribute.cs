namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class DisableComponentEnabledCheckboxAttribute : Attribute { }
}
