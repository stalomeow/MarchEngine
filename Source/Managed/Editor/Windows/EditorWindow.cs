using March.Core;
using March.Core.Interop;
using March.Core.Pool;
using Newtonsoft.Json;
using System.Numerics;

namespace March.Editor.Windows
{
    public enum DockNodeSplitDir
    {
        None = -1,
        Left = 0,
        Right = 1,
        Up = 2,
        Down = 3,
    }

    public abstract partial class EditorWindow : NativeMarchObject
    {
        private bool m_IsOnOpenInvoked = false;

        protected EditorWindow(nint nativePtr, string titleIcon, string titleContent) : base(nativePtr)
        {
            SetTitle(titleIcon, titleContent);
            SetRandomId();
        }

        protected EditorWindow(nint nativePtr, string title) : base(nativePtr)
        {
            SetTitle(title);
            SetRandomId();
        }

        protected EditorWindow(nint nativePtr) : base(nativePtr)
        {
            SetTitle(GetType().Name);
            SetRandomId();
        }

        protected EditorWindow(string titleIcon, string titleContent) : this(NewDefault(), titleIcon, titleContent) { }

        protected EditorWindow(string title) : this(NewDefault(), title) { }

        protected EditorWindow() : this(NewDefault()) { }

        protected sealed override void OnDispose(bool disposing)
        {
            if (m_IsOnOpenInvoked && IsOpen)
            {
                IsOpen = false;
                OnClose();
            }

            base.OnDispose(disposing);
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

        private void SetRandomId()
        {
            using var id = GuidUtility.BuildNew();
            SetId(id);
        }

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
                // Begin 可能修改 IsOpen 的值
                if (Begin() && IsOpen)
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
        private static partial nint NewDefault();

        [NativeMethod]
        private partial bool Begin();

        [NativeMethod]
        private partial void End();

        [NativeProperty]
        public static partial uint MainViewportDockSpaceNode { get; }

        [NativeMethod]
        public static partial void SplitDockNode(uint node, DockNodeSplitDir splitDir, float sizeRatioForNodeAtDir, out uint nodeAtDir, out uint nodeAtOppositeDir);

        [NativeMethod]
        public static partial void ApplyModificationsInChildDockNodes(uint rootNode);

        [NativeMethod]
        public partial void DockIntoNode(uint node);
    }
}
