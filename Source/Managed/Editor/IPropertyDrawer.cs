namespace March.Editor
{
    public interface IPropertyDrawer : IDrawer
    {
        /// <summary>
        /// 绘制 <paramref name="property"/>
        /// </summary>
        /// <param name="label"></param>
        /// <param name="tooltip"></param>
        /// <param name="property"></param>
        /// <returns>如果修改了 <paramref name="property"/> 的值，返回 <c>true</c>，否则返回 <c>false</c></returns>
        bool Draw(string label, string tooltip, in EditorProperty property);
    }

    public interface IPropertyDrawerFor<T> : IPropertyDrawer { }
}
