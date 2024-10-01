using March.Core.Serialization;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.Serialization;

namespace March.Core
{
    public sealed class GameObject : MarchObject, IForceInlineSerialization
    {
        [JsonProperty] private bool m_IsActive = false;
        [JsonProperty] internal List<Component> m_Components = [];

        [JsonProperty] public string Name { get; set; } = "New GameObject";

        [JsonProperty] public Transform Transform { get; private set; } = new();

        public GameObject()
        {
            IsActive = true;
        }

        public bool IsActive
        {
            get => m_IsActive;
            set
            {
                if (m_IsActive == value)
                {
                    return;
                }

                m_IsActive = value;

                foreach (var component in m_Components)
                {
                    component.OnDidMoutingObjectActiveStateChanged();
                }
            }
        }

        public T AddComponent<T>() where T : Component, new()
        {
            var component = new T();
            component.Mount(this, true);
            m_Components.Add(component);
            return component;
        }

        public Component AddComponent(Type type)
        {
            if (!type.IsSubclassOf(typeof(Component)))
            {
                throw new ArgumentException("Type must be a subclass of Component", nameof(type));
            }

            if (Activator.CreateInstance(type) is not Component component)
            {
                throw new NotSupportedException("Failed to create component");
            }

            component.Mount(this, true);
            m_Components.Add(component);
            return component;
        }

        public T? GetComponent<T>() where T : Component
        {
            foreach (var component in m_Components)
            {
                if (component is T t)
                {
                    return t;
                }
            }

            return null;
        }

        public bool TryGetComponent<T>([NotNullWhen(returnValue: true)] out T? component) where T : Component
        {
            component = GetComponent<T>();
            return component != null;
        }

        [OnDeserialized]
        private void InitializeAfterDeserialized(StreamingContext context)
        {
            foreach (var component in m_Components)
            {
                component.Mount(this, null);
            }
        }

        internal void Update()
        {
            foreach (var component in m_Components)
            {
                component.TryUpdate();
            }
        }
    }
}
