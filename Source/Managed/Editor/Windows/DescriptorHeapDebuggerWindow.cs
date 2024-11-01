using March.Core.Interop;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Descriptor Heap Debugger")]
    internal partial class DescriptorHeapDebuggerWindow : EditorWindow
    {
        public DescriptorHeapDebuggerWindow() : base(DescriptorHeapDebuggerWindow_New(), "Descriptor Heap Debugger") { }

        protected override void OnDispose(bool disposing)
        {
            DescriptorHeapDebuggerWindow_Delete(NativePtr);
            base.OnDispose(disposing);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint DescriptorHeapDebuggerWindow_New();

        [NativeFunction]
        private static partial void DescriptorHeapDebuggerWindow_Delete(nint w);

        #endregion
    }
}
