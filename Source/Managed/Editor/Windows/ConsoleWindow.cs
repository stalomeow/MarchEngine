using March.Core.Binding;
using Newtonsoft.Json;

namespace March.Editor.Windows
{
    [EditorWindowMenu("Window/General/Console")]
    internal partial class ConsoleWindow : EditorWindow
    {
        public ConsoleWindow() : base(ConsoleWindow_New(), "Console") { }

        protected override void OnDispose(bool disposing)
        {
            ConsoleWindow_Delete(NativePtr);
            base.OnDispose(disposing);
        }

        [JsonProperty]
        public int LogTypeFilter
        {
            get => ConsoleWindow_GetLogTypeFilter(NativePtr);
            set => ConsoleWindow_SetLogTypeFilter(NativePtr, value);
        }

        [JsonProperty]
        public bool EnableAutoScroll
        {
            get => ConsoleWindow_GetAutoScroll(NativePtr);
            set => ConsoleWindow_SetAutoScroll(NativePtr, value);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint ConsoleWindow_New();

        [NativeFunction]
        private static partial void ConsoleWindow_Delete(nint w);

        [NativeFunction]
        private static partial int ConsoleWindow_GetLogTypeFilter(nint w);

        [NativeFunction]
        private static partial void ConsoleWindow_SetLogTypeFilter(nint w, int value);

        [NativeFunction]
        private static partial bool ConsoleWindow_GetAutoScroll(nint w);

        [NativeFunction]
        private static partial void ConsoleWindow_SetAutoScroll(nint w, bool value);

        #endregion
    }
}
