using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Render Graph Debugger")]
    internal partial class RenderGraphDebuggerWindow : EditorWindow
    {
        public RenderGraphDebuggerWindow() : base(RenderGraphDebuggerWindow_New(), "Render Graph Debugger") { }

        protected override void OnDispose(bool disposing)
        {
            RenderGraphDebuggerWindow_Delete(NativePtr);
            base.OnDispose(disposing);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint RenderGraphDebuggerWindow_New();

        [NativeFunction]
        private static partial void RenderGraphDebuggerWindow_Delete(nint w);

        #endregion
    }
}
