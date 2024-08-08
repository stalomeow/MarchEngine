using Newtonsoft.Json;

namespace DX12Demo.Core
{
    public abstract class Component : EngineObject
    {
        [JsonProperty] private bool m_IsEnabled = false;
        private GameObject? m_MountingObject = null;

        protected Component() { }

        internal void Mount(GameObject mountingObject, bool? isEnabled)
        {
            if (m_MountingObject != null)
            {
                throw new InvalidOperationException("Component is already mounted");
            }

            m_MountingObject = mountingObject;
            OnMount();

            if (isEnabled != null)
            {
                IsEnabled = isEnabled.Value;
            }
            else if (m_IsEnabled)
            {
                OnEnable();
            }
            else
            {
                OnDisable();
            }
        }

        internal void Unmount()
        {
            if (m_MountingObject == null)
            {
                throw new InvalidOperationException("Component is not mounted");
            }

            IsEnabled = false;
            OnUnmount();
            m_MountingObject = null;
        }

        internal void TryUpdate()
        {
            if (!IsEnabled)
            {
                return;
            }

            OnUpdate();
        }

        internal void OnDidMoutingObjectActiveStateChanged()
        {
            InvokeOnEnableOrDisableCallback(!MountingObject.IsActive && IsEnabled);
        }

        public bool IsEnabled
        {
            get => m_IsEnabled;
            set
            {
                if (m_IsEnabled == value)
                {
                    return;
                }

                m_IsEnabled = value;
                InvokeOnEnableOrDisableCallback(MountingObject.IsActive && !value);
            }
        }

        public bool IsActiveAndEnabled => MountingObject.IsActive && IsEnabled;

        public GameObject MountingObject => m_MountingObject!;

        private void InvokeOnEnableOrDisableCallback(bool oldIsActiveAndEnabled)
        {
            bool newIsActiveAndEnabled = IsActiveAndEnabled;
            if (oldIsActiveAndEnabled == newIsActiveAndEnabled)
            {
                return;
            }

            if (newIsActiveAndEnabled)
            {
                OnEnable();
            }
            else
            {
                OnDisable();
            }
        }

        protected virtual void OnMount()
        {
            Debug.LogInfo($"Component {GetType().FullName} is mounted");
        }

        protected virtual void OnUnmount()
        {
            Debug.LogInfo($"Component {GetType().FullName} is unmounted");
        }

        protected virtual void OnEnable()
        {
            Debug.LogInfo($"Component {GetType().FullName} is enabled");
        }

        protected virtual void OnDisable()
        {
            Debug.LogInfo($"Component {GetType().FullName} is disabled");
        }

        protected virtual void OnUpdate() { }
    }
}
