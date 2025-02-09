using March.Core.IconFont;
using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Graphics Debugger")]
    internal partial class GraphicsDebuggerWindow : EditorWindow
    {
        public GraphicsDebuggerWindow() : base(New(), FontAwesome6.BugSlash, "Graphics Debugger") { }

        [NativeMethod]
        private static partial nint New();
    }
}
