using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Descriptor Heap Debugger")]
    internal partial class DescriptorHeapDebuggerWindow : EditorWindow
    {
        public DescriptorHeapDebuggerWindow() : base(New(), "Descriptor Heap Debugger") { }

        [NativeMethod]
        private static partial nint New();
    }
}
