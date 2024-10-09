using March.Core;
using March.Core.Binding;
using March.Core.Serialization;

namespace March.Editor.Windows
{
    public abstract partial class EditorWindow : NativeMarchObject, IForceInlineSerialization
    {
        private readonly bool m_IsDefaultNativeWindow;
        private bool m_IsOnOpenInvoked = false;

        protected EditorWindow() : base(EditorWindow_CreateDefault())
        {
            m_IsDefaultNativeWindow = true;
        }

        protected EditorWindow(nint nativePtr) : base(nativePtr)
        {
            m_IsDefaultNativeWindow = false;
        }

        protected EditorWindow(string title) : this()
        {
            Title = title;
        }

        protected EditorWindow(nint nativePtr, string title) : this(nativePtr)
        {
            Title = title;
        }

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

            set
            {
                using NativeString v = value;
                EditorWindow_SetTitle(NativePtr, v.Data);
            }
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
