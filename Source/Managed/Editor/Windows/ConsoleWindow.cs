using March.Core.IconFont;
using March.Core.Interop;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Console")]
    internal partial class ConsoleWindow : EditorWindow
    {
        public ConsoleWindow() : base(New(), FontAwesome6.Terminal, "Console")
        {
            DefaultSize = new Vector2(850.0f, 400.0f);
        }

        protected override void OnDispose(bool disposing)
        {
            Delete();
            base.OnDispose(disposing);
        }

        [JsonProperty]
        [NativeProperty]
        public partial int LogTypeFilter { get; set; }

        [JsonProperty]
        [NativeProperty]
        public partial bool EnableAutoScroll { get; set; }

        [NativeMethod]
        private static partial nint New();

        [NativeMethod]
        private partial void Delete();
    }
}
