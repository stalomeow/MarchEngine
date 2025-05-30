using March.Core.Serialization;
using Newtonsoft.Json;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.Serialization;
using System.Text.RegularExpressions;

namespace March.Core
{
    public sealed class GameObject : MarchObject, IDisposable
    {
        private const string k_NameCharBlacklist = "/\\:;*?\"<>|";

        [JsonProperty]
        [StringDrawer(CharBlacklist = k_NameCharBlacklist)]
        private string m_Name = "New GameObject";

        [JsonProperty] private bool m_IsActive = true;
        [JsonProperty] private Transform m_Transform = new();
        [JsonProperty] internal List<Component> m_Components = [];
        private bool m_IsAwaked = false;

        internal GameObject()
        {
            m_Transform.Initialize(this);
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext context)
        {
            m_Transform.Initialize(this);

            foreach (var component in m_Components)
            {
                component.Initialize(this);
            }
        }

        public string Name
        {
            get => m_Name;
            set
            {
                string regex = $"[{Regex.Escape(k_NameCharBlacklist)}]";
                m_Name = Regex.Replace(value, regex, string.Empty);
            }
        }

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

                if (m_IsAwaked)
                {
                    TryMountExistingComponentsRecursive();
                    TryEnableOrDisableExistingComponentsRecursive();
                }
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

        public Transform transform => m_Transform;

        public T AddComponent<T>() where T : Component, new()
        {
            if (typeof(T) == typeof(Transform))
            {
                throw new InvalidOperationException("Transform component is already attached to GameObject");
            }

            var component = new T();
            AddComponentImpl(component);
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

            AddComponentImpl(component);
            return component;
        }

        private void AddComponentImpl(Component component)
        {
            component.Initialize(this);
            m_Components.Add(component);

            if (m_IsAwaked)
            {
                component.TryMount();
                component.TryEnableOrDisable();
            }
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

        public void DestroyAndRemoveComponent(Component component)
        {
            if (component.gameObject != this)
            {
                throw new ArgumentException("Component is not attached to this GameObject", nameof(component));
            }

            if (component.GetType() == typeof(Transform))
            {
                throw new InvalidOperationException("Transform component cannot be removed from GameObject");
            }

            component.Dispose();
            m_Components.Remove(component);
        }

        /// <summary>
        /// 递归销毁 <see cref="GameObject"/> 及其所有子 <see cref="GameObject"/> 和 <see cref="Component"/>
        /// </summary>
        public void Dispose()
        {
            DisposeRecursive();
        }

        private void DisposeRecursive()
        {
            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.DisposeRecursive();
            }

            m_Transform.Dispose();

            foreach (Component component in m_Components)
            {
                component.Dispose();
            }

            m_Components.Clear();
        }

        internal void AwakeRecursive()
        {
            SetAwakeRecursive();
            TryMountExistingComponentsRecursive();
            TryEnableOrDisableExistingComponentsRecursive();
        }

        private void SetAwakeRecursive()
        {
            if (m_IsAwaked)
            {
                throw new InvalidOperationException("GameObject is already awaked");
            }

            m_IsAwaked = true;

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.SetAwakeRecursive();
            }
        }

        private void TryMountExistingComponentsRecursive()
        {
            m_Transform.TryMount();

            foreach (var component in m_Components)
            {
                component.TryMount();
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.TryMountExistingComponentsRecursive();
            }
        }

        private void TryEnableOrDisableExistingComponentsRecursive()
        {
            m_Transform.TryEnableOrDisable();

            foreach (var component in m_Components)
            {
                component.TryEnableOrDisable();
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.TryEnableOrDisableExistingComponentsRecursive();
            }
        }

        internal void UpdateRecursive()
        {
            if (!m_IsAwaked)
            {
                return;
            }

            m_Transform.TryUpdate();

            foreach (var component in m_Components)
            {
                component.TryUpdate();
            }

            for (int i = 0; i < m_Transform.ChildCount; i++)
            {
                m_Transform.GetChild(i).gameObject.UpdateRecursive();
            }
        }
    }
}
