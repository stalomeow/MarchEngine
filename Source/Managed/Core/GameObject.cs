using March.Core.Serialization;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;

namespace March.Core
{
    public sealed class GameObject : MarchObject, IForceInlineSerialization
    {
        [JsonProperty] private bool m_IsActive = true;
        [JsonProperty] private Transform m_Transform = new();
        [JsonProperty] internal List<Component> m_Components = [];

        [JsonProperty] public string Name { get; set; } = "New GameObject";

        public Transform transform => m_Transform;

        public bool IsActiveSelf
        {
            get => m_IsActive;
            set
            {
                if (m_IsActive == value)
                {
                    return;
                }

                m_IsActive = value;
                NotifyActiveStateChangedRecursive();
            }
        }

        public bool IsActiveInHierarchy
        {
            get
            {
                if (!m_IsActive)
                {
                    return false;
                }

                Transform? parent = m_Transform.Parent;
                return parent == null || parent.gameObject.IsActiveInHierarchy;
            }
        }

        public T AddComponent<T>() where T : Component, new()
        {
            if (typeof(T) == typeof(Transform))
            {
                throw new InvalidOperationException("Transform component is already attached to GameObject");
            }

            var component = new T();
            m_Components.Add(component);
            component.Mount(this);
            component.PostMount();
            return component;
        }

        public Component AddComponent(Type type)
        {
            if (!type.IsSubclassOf(typeof(Component)))
            {
                throw new ArgumentException("Type must be a subclass of Component", nameof(type));
            }

            if (type == typeof(Transform))
            {
                throw new InvalidOperationException("Transform component is already attached to GameObject");
            }

            if (Activator.CreateInstance(type) is not Component component)
            {
                throw new NotSupportedException("Failed to create component");
            }

            m_Components.Add(component);
            component.Mount(this);
            component.PostMount();
            return component;
        }

        public T? GetComponent<T>() where T : Component
        {
            if (typeof(T) == typeof(Transform))
            {
                return Unsafe.As<Transform, T>(ref m_Transform);
            }

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

        // TODO check is active

        internal void AwakeRecursive()
        {
            MountExistingComponentsRecursive();
            PostMountExistingComponentsRecursive();
        }

        private void MountExistingComponentsRecursive()
        {
            m_Transform.Mount(this);

            foreach (var component in m_Components)
            {
                component.Mount(this);
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.MountExistingComponentsRecursive();
            }
        }

        private void PostMountExistingComponentsRecursive()
        {
            m_Transform.PostMount();

            foreach (var component in m_Components)
            {
                component.PostMount();
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.PostMountExistingComponentsRecursive();
            }
        }

        private void NotifyActiveStateChangedRecursive()
        {
            m_Transform.OnGameObjectActiveStateChanged();

            foreach (var component in m_Components)
            {
                component.OnGameObjectActiveStateChanged();
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.NotifyActiveStateChangedRecursive();
            }
        }

        internal void UpdateRecursive()
        {
            m_Transform.Update();

            foreach (var component in m_Components)
            {
                component.Update();
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.UpdateRecursive();
            }
        }
    }
}
