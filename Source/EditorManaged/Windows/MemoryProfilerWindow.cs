using March.Core.Interop;
using March.Editor.IconFont;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Memory Profiler")]
    internal partial class MemoryProfilerWindow : EditorWindow
    {
        public MemoryProfilerWindow() : base(New(), FontAwesome6.MagnifyingGlass, "Memory Profiler") { }

        [NativeMethod]
        private static partial nint New();
    }
}
