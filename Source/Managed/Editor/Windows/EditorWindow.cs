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

        protected EditorWindow(string titleIcon, string titleContent) : this(CreateDefault(), true, titleIcon, titleContent) { }

        protected EditorWindow(string title) : this(CreateDefault(), true, null, title) { }

        protected EditorWindow() : this(CreateDefault(), true, null, null) { }

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
                DeleteDefault();
            }
        }

        [NativeProperty]
        public partial string Title { get; }

        [JsonProperty]
        [NativeProperty]
        public partial string Id { get; protected internal set; }

        [NativeProperty]
        public partial Vector2 DefaultSize { get; protected set; }

        [NativeProperty]
        internal partial bool IsOpen { get; private set; }

        protected void SetTitle(StringLike icon, StringLike content)
        {
            using var title = StringBuilderPool.Get();
            title.Value.Append(icon).Append(' ').Append(content);
            SetTitle(title);
        }

        [NativeMethod]
        protected partial void SetTitle(StringLike value);

        [NativeMethod]
        protected internal partial void SetId(StringLike value);

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
                if (Begin())
                {
                    OnDraw();
                }
            }
            finally
            {
                End();

                if (!IsOpen)
                {
                    OnClose();
                }
            }
        }

        [NativeMethod]
        protected virtual partial void OnOpen();

        [NativeMethod]
        protected virtual partial void OnClose();

        [NativeMethod]
        protected virtual partial void OnDraw();

        [NativeMethod]
        private static partial nint CreateDefault();

        [NativeMethod]
        private partial void DeleteDefault();

        [NativeMethod]
        private partial bool Begin();

        [NativeMethod]
        private partial void End();
    }
}
