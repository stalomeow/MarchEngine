using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Graphics Debugger")]
    internal partial class GraphicsDebuggerWindow : EditorWindow
    {
        public GraphicsDebuggerWindow() : base(New(), "Graphics Debugger") { }

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
