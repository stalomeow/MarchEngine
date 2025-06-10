using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Dear ImGui Demo")]
    internal partial class ImGuiDemoWindow : EditorWindow
    {
        public ImGuiDemoWindow() : base(New(), "Dear ImGui Demo") { }

        [NativeMethod]
        private static partial nint New();
    }
}
