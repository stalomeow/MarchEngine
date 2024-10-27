using March.Core;
using March.Core.Binding;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    public abstract partial class EditorWindow : NativeMarchObject
    {
        private readonly bool m_IsDefaultNativeWindow;
        private bool m_IsOnOpenInvoked = false;

        private EditorWindow(nint nativePtr, bool isDefaultNativeWindow, string? title) : base(nativePtr)
        {
            m_IsDefaultNativeWindow = isDefaultNativeWindow;
            Title = title ?? GetType().Name;
            Id = Guid.NewGuid().ToString("N");
        }

        protected EditorWindow(string? title = null) : this(EditorWindow_CreateDefault(), true, title) { }

        protected EditorWindow(nint nativePtr, string? title = null) : this(nativePtr, false, title) { }

        protected sealed override void Dispose(bool disposing)
        {
            if (m_IsOnOpenInvoked && IsOpen)
            {
                IsOpen = false;
                OnClose();
            }

            OnDispose(disposing);
        }

        protected virtual void OnDispose(bool disposing)
        {
            if (m_IsDefaultNativeWindow)
            {
                EditorWindow_DeleteDefault(NativePtr);
            }
        }

        public string Title
        {
            get
            {
                nint title = EditorWindow_GetTitle(NativePtr);
                return NativeString.GetAndFree(title);
            }

            protected set
            {
                using NativeString v = value;
                EditorWindow_SetTitle(NativePtr, v.Data);
            }
        }

        [JsonProperty]
        public string Id
        {
            get
            {
                nint id = EditorWindow_GetId(NativePtr);
                return NativeString.GetAndFree(id);
            }

            protected internal set
            {
                using NativeString v = value;
                EditorWindow_SetId(NativePtr, v.Data);
            }
        }

        public Vector2 DefaultSize
        {
            get => EditorWindow_GetDefaultSize(NativePtr);
            protected set => EditorWindow_SetDefaultSize(NativePtr, value);
        }

        internal bool IsOpen
        {
            get => EditorWindow_GetIsOpen(NativePtr);
            private set => EditorWindow_SetIsOpen(NativePtr, value);
        }

        internal void Draw()
        {
            if (!IsOpen)
            {
                return;
            }

            if (!m_IsOnOpenInvoked)
            {
                m_IsOnOpenInvoked = true;
                OnOpen();
            }

            try
            {
                if (EditorWindow_Begin(NativePtr))
                {
                    OnDraw();
                }
            }
            finally
            {
                EditorWindow_End(NativePtr);

                if (!IsOpen)
                {
                    OnClose();
                }
            }
        }

        protected virtual void OnOpen()
        {
            EditorWindow_OnOpen(NativePtr);
        }

        protected virtual void OnClose()
        {
            EditorWindow_OnClose(NativePtr);
        }

        protected virtual void OnDraw()
        {
            EditorWindow_OnDraw(NativePtr);
        }

        #region Bindings

        [NativeFunction]
        private static partial nint EditorWindow_CreateDefault();

        [NativeFunction]
        private static partial void EditorWindow_DeleteDefault(nint w);

        [NativeFunction]
        private static partial nint EditorWindow_GetTitle(nint w);

        [NativeFunction]
        private static partial void EditorWindow_SetTitle(nint w, nint title);

        [NativeFunction]
        private static partial nint EditorWindow_GetId(nint w);

        [NativeFunction]
        private static partial void EditorWindow_SetId(nint w, nint id);

        [NativeFunction]
        private static partial Vector2 EditorWindow_GetDefaultSize(nint w);

        [NativeFunction]
        private static partial void EditorWindow_SetDefaultSize(nint w, Vector2 value);

        [NativeFunction]
        private static partial bool EditorWindow_GetIsOpen(nint w);

        [NativeFunction]
        private static partial void EditorWindow_SetIsOpen(nint w, bool value);

        [NativeFunction]
        private static partial bool EditorWindow_Begin(nint w);

        [NativeFunction]
        private static partial void EditorWindow_End(nint w);

        [NativeFunction]
        private static partial void EditorWindow_OnOpen(nint w);

        [NativeFunction]
        private static partial void EditorWindow_OnClose(nint w);

        [NativeFunction]
        private static partial void EditorWindow_OnDraw(nint w);

        #endregion
    }
}
