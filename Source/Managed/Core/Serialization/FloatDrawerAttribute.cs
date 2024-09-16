namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false)]
    public sealed class FloatDrawerAttribute : Attribute
    {
        public float DragSpeed { get; set; } = 0.1f;

        public float Min { get; set; } = float.MinValue;

        public float Max { get; set; } = float.MaxValue;
    }
}
