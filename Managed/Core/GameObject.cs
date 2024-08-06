using System.Diagnostics.CodeAnalysis;
using System.Numerics;

namespace DX12Demo.Core
{
    public sealed class GameObject
    {
        private bool m_IsActive = false;
        private readonly List<Component> m_Components = [];

        public string Name { get; set; } = "New GameObject";

        public Vector3 Position { get; set; } = Vector3.Zero;

        public Quaternion Rotation { get; set; } = Quaternion.Identity;

        public Vector3 Scale { get; set; } = Vector3.One;

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

        internal void Update()
        {
            foreach (var component in m_Components)
            {
                component.TryUpdate();
            }
        }
    }
}
