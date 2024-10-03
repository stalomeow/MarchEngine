using March.Core.Binding;
using March.Core.Serialization;
using Newtonsoft.Json;

namespace March.Core
{
    public abstract partial class Component : NativeMarchObject, IForceInlineSerialization
    {
        private readonly bool m_IsDefaultNativeComponent;
        private bool m_IsEnabled = true;
        private GameObject? m_GameObject = null;

        protected Component() : base(Component_CreateDefault())
        {
            m_IsDefaultNativeComponent = true;
        }

        protected Component(nint nativePtr) : base(nativePtr)
        {
            m_IsDefaultNativeComponent = false;
        }

        protected override void Dispose(bool disposing)
        {
            if (m_IsDefaultNativeComponent)
            {
                Component_DeleteDefault(NativePtr);
            }
        }

        public GameObject gameObject => m_GameObject!;

        [InspectorName("Enable")]
        [JsonProperty(Order = -9999)] // 显示在最前面
        public bool IsEnabled
        {
            get => m_IsEnabled;
            set
            {
                m_IsEnabled = value;
                InvokeScriptEnabledCallback();
            }
        }

        public bool IsActiveAndEnabled => Component_GetIsActiveAndEnabled(NativePtr);

        internal void Mount(GameObject gameObject)
        {
            if (m_GameObject != null)
            {
                throw new InvalidOperationException("Component has been mounted to a GameObject");
            }

            m_GameObject = gameObject;
            Component_SetTransform(NativePtr, gameObject.transform.NativePtr);
            OnMount();
        }

        internal void PostMount()
        {
            InvokeScriptEnabledCallback();
        }

        internal void PreUnmount()
        {
            IsEnabled = false;
        }

        internal void UnmountAndDispose()
        {
            if (m_GameObject == null)
            {
                throw new InvalidOperationException("Component has not been mounted to any GameObject");
            }

            if (IsEnabled)
            {
                throw new InvalidOperationException($"Component is still enabled; call {nameof(PreUnmount)} please");
            }

            OnUnmount();
            m_GameObject = null;

            Dispose();
        }

        internal void Update()
        {
            if (IsActiveAndEnabled)
            {
                OnUpdate();
            }
        }

        internal void OnGameObjectActiveStateChanged()
        {
            InvokeScriptEnabledCallback();
        }

        private void InvokeScriptEnabledCallback()
        {
            if (m_GameObject == null)
            {
                return;
            }

            bool willActiveAndEnabled = IsEnabled && gameObject.IsActiveInHierarchy;
            if (willActiveAndEnabled == IsActiveAndEnabled)
            {
                return;
            }

            if (willActiveAndEnabled)
            {
                Component_SetIsActiveAndEnabled(NativePtr, true);
                OnEnable();
            }
            else
            {
                Component_SetIsActiveAndEnabled(NativePtr, false);
                OnDisable();
            }
        }

        protected virtual void OnMount() => Component_OnMount(NativePtr);

        protected virtual void OnUnmount() => Component_OnUnmount(NativePtr);

        protected virtual void OnEnable() => Component_OnEnable(NativePtr);

        protected virtual void OnDisable() => Component_OnDisable(NativePtr);

        protected virtual void OnUpdate() => Component_OnUpdate(NativePtr);

        #region Bindings

        [NativeFunction]
        private static partial nint Component_CreateDefault();

        [NativeFunction]
        private static partial void Component_DeleteDefault(nint component);

        [NativeFunction]
        private static partial bool Component_GetIsActiveAndEnabled(nint component);

        [NativeFunction]
        private static partial void Component_SetIsActiveAndEnabled(nint component, bool value);

        [NativeFunction]
        private static partial void Component_SetTransform(nint component, nint transform);

        [NativeFunction]
        private static partial void Component_OnMount(nint component);

        [NativeFunction]
        private static partial void Component_OnUnmount(nint component);

        [NativeFunction]
        private static partial void Component_OnEnable(nint component);

        [NativeFunction]
        private static partial void Component_OnDisable(nint component);

        [NativeFunction]
        private static partial void Component_OnUpdate(nint component);

        #endregion
    }
}
