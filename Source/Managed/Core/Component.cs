using March.Core.Binding;
using March.Core.Serialization;
using Newtonsoft.Json;
using System.ComponentModel;

namespace March.Core
{
    public abstract partial class Component : NativeMarchObject, IForceInlineSerialization
    {
        private readonly bool m_IsDefaultNativeComponent;
        private bool m_IsScriptEnabled = false; // 如果为 true 说明调用过 OnEnable()
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

        [JsonProperty(nameof(IsEnabled))]
        [EditorBrowsable(EditorBrowsableState.Never)]
        private bool IsEnabledSerializationOnly
        {
            get => Component_GetIsActive(NativePtr);
            set => Component_SetIsActive(NativePtr, value);
        }

        public bool IsEnabled
        {
            get => Component_GetIsActive(NativePtr);
            set
            {
                Component_SetIsActive(NativePtr, value);
                InvokeScriptEnabledCallback();
            }
        }

        public bool IsActiveAndEnabled => IsEnabled && gameObject.IsActiveInHierarchy;

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

            if (m_IsScriptEnabled)
            {
                throw new InvalidOperationException("Component is still enabled");
            }

            OnUnmount();
            m_GameObject = null;

            Dispose();
        }

        internal void Update()
        {
            if (m_IsScriptEnabled)
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
            bool isActiveAndEnabled = IsActiveAndEnabled;
            if (m_IsScriptEnabled == isActiveAndEnabled)
            {
                return;
            }

            if (isActiveAndEnabled)
            {
                m_IsScriptEnabled = true;
                OnEnable();
            }
            else
            {
                m_IsScriptEnabled = false;
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
        private static partial bool Component_GetIsActive(nint component);

        [NativeFunction]
        private static partial void Component_SetIsActive(nint component, bool value);

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
