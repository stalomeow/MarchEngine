using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    public abstract partial class EditorWindow : NativeMarchObject
    {
        private readonly bool m_IsDefaultNativeWindow;
        private bool m_IsOnOpenInvoked = false;

        private EditorWindow(nint nativePtr, bool isDefaultNativeWindow, string? titleIcon, string? titleContent) : base(nativePtr)
        {
            m_IsDefaultNativeWindow = isDefaultNativeWindow;

            titleContent ??= GetType().Name;

            if (titleIcon == null)
            {
                SetTitle(titleContent);
            }
            else
            {
                SetTitle(titleIcon, titleContent);
            }

            using var id = GuidUtility.BuildNew();
            SetId(id);
        }

        protected EditorWindow(string titleIcon, string titleContent) : this(EditorWindow_CreateDefault(), true, titleIcon, titleContent) { }

        protected EditorWindow(string title) : this(EditorWindow_CreateDefault(), true, null, title) { }

        protected EditorWindow() : this(EditorWindow_CreateDefault(), true, null, null) { }

        protected EditorWindow(nint nativePtr, string titleIcon, string titleContent) : this(nativePtr, false, titleIcon, titleContent) { }

        protected EditorWindow(nint nativePtr, string title) : this(nativePtr, false, null, title) { }

        protected EditorWindow(nint nativePtr) : this(nativePtr, false, null, null) { }

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
        }

        [JsonProperty]
        public string Id
        {
            get
            {
                nint id = EditorWindow_GetId(NativePtr);
                return NativeString.GetAndFree(id);
            }

            protected internal set => SetId(value);
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

        protected void SetTitle(StringLike icon, StringLike content)
        {
            using var title = StringBuilderPool.Get();
            title.Value.Append(icon).Append(' ').Append(content);
            SetTitle(title);
        }

        protected void SetTitle(StringLike value)
        {
            using NativeString v = value;
            EditorWindow_SetTitle(NativePtr, v.Data);
        }

        protected internal void SetId(StringLike value)
        {
            using NativeString v = value;
            EditorWindow_SetId(NativePtr, v.Data);
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

        [NativeMethod]
        private static partial nint EditorWindow_CreateDefault();

        [NativeMethod]
        private static partial void EditorWindow_DeleteDefault(nint w);

        [NativeMethod]
        private static partial nint EditorWindow_GetTitle(nint w);

        [NativeMethod]
        private static partial void EditorWindow_SetTitle(nint w, nint title);

        [NativeMethod]
        private static partial nint EditorWindow_GetId(nint w);

        [NativeMethod]
        private static partial void EditorWindow_SetId(nint w, nint id);

        [NativeMethod]
        private static partial Vector2 EditorWindow_GetDefaultSize(nint w);

        [NativeMethod]
        private static partial void EditorWindow_SetDefaultSize(nint w, Vector2 value);

        [NativeMethod]
        private static partial bool EditorWindow_GetIsOpen(nint w);

        [NativeMethod]
        private static partial void EditorWindow_SetIsOpen(nint w, bool value);

        [NativeMethod]
        private static partial bool EditorWindow_Begin(nint w);

        [NativeMethod]
        private static partial void EditorWindow_End(nint w);

        [NativeMethod]
        private static partial void EditorWindow_OnOpen(nint w);

        [NativeMethod]
        private static partial void EditorWindow_OnClose(nint w);

        [NativeMethod]
        private static partial void EditorWindow_OnDraw(nint w);

        #endregion
    }
}
