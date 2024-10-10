using March.Core.Binding;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Dear ImGui Demo")]
    internal partial class ImGuiDemoWindow : EditorWindow
    {
        public ImGuiDemoWindow() : base(ImGuiDemoWindow_New(), "Dear ImGui Demo") { }

        protected override void OnDispose(bool disposing)
        {
            ImGuiDemoWindow_Delete(NativePtr);
            base.OnDispose(disposing);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint ImGuiDemoWindow_New();

        [NativeFunction]
        private static partial void ImGuiDemoWindow_Delete(nint w);

        #endregion
    }
}
