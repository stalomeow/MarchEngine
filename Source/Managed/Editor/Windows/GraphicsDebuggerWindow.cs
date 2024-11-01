using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Graphics Debugger")]
    internal partial class GraphicsDebuggerWindow : EditorWindow
    {
        public GraphicsDebuggerWindow() : base(GraphicsDebuggerWindow_New(), "Graphics Debugger") { }

        protected override void OnDispose(bool disposing)
        {
            GraphicsDebuggerWindow_Delete(NativePtr);
            base.OnDispose(disposing);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint GraphicsDebuggerWindow_New();

        [NativeFunction]
        private static partial void GraphicsDebuggerWindow_Delete(nint w);

        #endregion
    }
}
