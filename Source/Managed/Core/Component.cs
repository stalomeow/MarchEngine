using March.Core.Interop;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    public abstract partial class Component : NativeMarchObject
    {
        private readonly bool m_IsDefaultNativeComponent;
        private bool m_IsMounted;

        [JsonProperty]
        [HideInInspector]
        private GameObject? m_GameObject;

        [JsonProperty]
        [HideInInspector]
        private bool m_IsEnabled = true;

        protected Component() : base(Component_CreateDefault()) => m_IsDefaultNativeComponent = true;

        /// <summary>
        /// 初始化一个自定义的 Native Component
        /// </summary>
        /// <param name="nativePtr"></param>
        /// <remarks>如果使用这个构造方法，必须重写 <see cref="DisposeNative"/> 释放非托管资源</remarks>
        protected Component(nint nativePtr) : base(nativePtr) => m_IsDefaultNativeComponent = false;

        protected sealed override void Dispose(bool disposing)
        {
            IsEnabled = false;

            if (m_IsMounted)
            {
                m_IsMounted = false;
                OnUnmount();
            }

            DisposeNative();
        }

        protected virtual void DisposeNative()
        {
            if (m_IsDefaultNativeComponent)
            {
                Component_DeleteDefault(NativePtr);
            }
        }

        public bool IsEnabled
        {
            get => m_IsEnabled;
            set
            {
                m_IsEnabled = value;
                TryEnableOrDisable();
            }
        }

        public bool IsActiveAndEnabled => Component_GetIsActiveAndEnabled(NativePtr);

        public GameObject gameObject => m_GameObject!;

        public Transform transform => gameObject.transform;

        /// <summary>
        /// 在创建组件后立刻调用，初始化一些必要的字段
        /// </summary>
        /// <param name="go"></param>
        internal void Initialize(GameObject go)
        {
            m_GameObject = go;
            m_IsMounted = false;

            Component_SetTransform(NativePtr, go.transform.NativePtr);
        }

        internal void TryMount()
        {
            if (!m_IsMounted && gameObject.IsActiveInHierarchy)
            {
                m_IsMounted = true;
                OnMount();
            }
        }

        internal void TryEnableOrDisable()
        {
            if (!m_IsMounted)
            {
                return;
            }

            bool willActiveAndEnabled = IsEnabled && gameObject.IsActiveInHierarchy;

            if (willActiveAndEnabled != IsActiveAndEnabled)
            {
                Component_SetIsActiveAndEnabled(NativePtr, willActiveAndEnabled);

                if (willActiveAndEnabled)
                {
                    OnEnable();
                }
                else
                {
                    OnDisable();
                }
            }
        }

        internal void TryUpdate()
        {
            if (m_IsMounted && IsActiveAndEnabled)
            {
                OnUpdate();
            }
        }

        internal void TryDrawGizmos(bool isSelected)
        {
            if (m_IsMounted && IsActiveAndEnabled)
            {
                OnDrawGizmos(isSelected);
            }
        }

        internal void TryDrawGizmosGUI(bool isSelected)
        {
            if (m_IsMounted && IsActiveAndEnabled)
            {
                OnDrawGizmosGUI(isSelected);
            }
        }

        /// <summary>
        /// 在组件被挂载且 <see cref="GameObject"/> 处于激活状态时被调用，只被调用一次；
        /// 即使 <see cref="Component"/> 没有被启用，这个方法也会被调用
        /// </summary>
        /// <remarks>这个方法在 <see cref="OnEnable"/> 前被调用，可以当成构造方法使用</remarks>
        protected virtual void OnMount() => Component_OnMount(NativePtr);

        /// <summary>
        /// 在组件被移除时调用；如果 <see cref="OnMount"/> 没有被调用过，这个方法也不会被调用；
        /// 即使 <see cref="GameObject"/> 没有被激活、<see cref="Component"/> 没有被启用，这个方法也会被调用
        /// </summary>
        /// <remarks>这个方法在 <see cref="OnDisable"/> 后被调用，可以当成析构方法使用</remarks>
        protected virtual void OnUnmount() => Component_OnUnmount(NativePtr);

        protected virtual void OnEnable() => Component_OnEnable(NativePtr);

        protected virtual void OnDisable() => Component_OnDisable(NativePtr);

        protected virtual void OnUpdate() => Component_OnUpdate(NativePtr);

        protected virtual void OnDrawGizmos(bool isSelected) => Component_OnDrawGizmos(NativePtr, isSelected);

        protected virtual void OnDrawGizmosGUI(bool isSelected) => Component_OnDrawGizmosGUI(NativePtr, isSelected);

        #region Bindings

        [NativeMethod]
        private static partial nint Component_CreateDefault();

        [NativeMethod]
        private static partial void Component_DeleteDefault(nint component);

        [NativeMethod]
        private static partial bool Component_GetIsActiveAndEnabled(nint component);

        [NativeMethod]
        private static partial void Component_SetIsActiveAndEnabled(nint component, bool value);

        [NativeMethod]
        private static partial void Component_SetTransform(nint component, nint transform);

        [NativeMethod]
        private static partial void Component_OnMount(nint component);

        [NativeMethod]
        private static partial void Component_OnUnmount(nint component);

        [NativeMethod]
        private static partial void Component_OnEnable(nint component);

        [NativeMethod]
        private static partial void Component_OnDisable(nint component);

        [NativeMethod]
        private static partial void Component_OnUpdate(nint component);

        [NativeMethod]
        private static partial void Component_OnDrawGizmos(nint component, bool isSelected);

        [NativeMethod]
        private static partial void Component_OnDrawGizmosGUI(nint component, bool isSelected);

        #endregion
    }
}
