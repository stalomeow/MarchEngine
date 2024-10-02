#pragma once

namespace march
{
    class Transform;
    class ComponentInternalUtility;

    class Component
    {
        friend ComponentInternalUtility;

    public:
        Component() = default;
        virtual ~Component() = default;

        Component(const Component&) = delete;
        Component& operator=(const Component&) = delete;
        Component(Component&&) = delete;
        Component& operator=(Component&&) = delete;

        bool GetIsEnabled() const { return m_IsEnabled; }
        Transform* GetTransform() const { return m_Transform; }

    protected:
        virtual void OnMount() {}
        virtual void OnUnmount() {}
        virtual void OnEnable() {}
        virtual void OnDisable() {}
        virtual void OnUpdate() {}

    private:
        bool m_IsEnabled = true;
        Transform* m_Transform = nullptr;
    };

    // C++ 侧提供给 C# 侧的接口，不要用它
    class ComponentInternalUtility
    {
    public:
        static void SetIsEnabled(Component* component, bool value) { component->m_IsEnabled = value; }
        static void SetTransform(Component* component, Transform* value) { component->m_Transform = value; }
        static void InvokeOnMount(Component* component) { component->OnMount(); }
        static void InvokeOnUnmount(Component* component) { component->OnUnmount(); }
        static void InvokeOnEnable(Component* component) { component->OnEnable(); }
        static void InvokeOnDisable(Component* component) { component->OnDisable(); }
        static void InvokeOnUpdate(Component* component) { component->OnUpdate(); }
    };
}
