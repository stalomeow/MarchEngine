using March.Core.IconFont;
using March.Core.Interop;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/Analysis/Render Graph Viewer")]
    internal partial class RenderGraphViewerWindow : EditorWindow
    {
        public RenderGraphViewerWindow() : base(New(), FontAwesome6.DiagramProject, "Render Graph Viewer")
        {
            DefaultSize = new Vector2(1000.0f, 650.0f);
        }

        [NativeMethod]
        private static partial nint New();
    }
}
