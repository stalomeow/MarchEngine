using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Render Graph Debugger")]
    internal partial class RenderGraphDebuggerWindow : EditorWindow
    {
        public RenderGraphDebuggerWindow() : base(New(), "Render Graph Debugger") { }

        protected override void OnDispose(bool disposing)
        {
            Delete();
            base.OnDispose(disposing);
        }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void Delete();
    }
}
