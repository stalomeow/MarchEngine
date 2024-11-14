using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Dear ImGui Demo")]
    internal partial class ImGuiDemoWindow : EditorWindow
    {
        public ImGuiDemoWindow() : base(New(), "Dear ImGui Demo") { }

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
