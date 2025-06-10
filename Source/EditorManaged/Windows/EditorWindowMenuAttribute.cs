namespace March.Editor.Windows
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
    public sealed class EditorWindowMenuAttribute(string menuPath) : Attribute
    {
        public string MenuPath { get; } = menuPath;
    }
}
