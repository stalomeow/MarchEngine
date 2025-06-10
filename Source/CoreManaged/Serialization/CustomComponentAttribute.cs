namespace March.Core.Serialization
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    public sealed class CustomComponentAttribute : Attribute
    {
        /// <summary>
        /// 不允许在 Inspector 上禁用组件
        /// </summary>
        public bool DisableCheckbox { get; set; } = false;

        /// <summary>
        /// 不允许在 Inspector 上删除组件
        /// </summary>
        public bool HideRemoveButton { get; set; } = false;
    }
}
