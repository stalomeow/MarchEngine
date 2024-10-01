#pragma once

namespace march
{
    class ComponentInternalUtility;

    class Component
    {
        friend ComponentInternalUtility;

    public:
        bool GetIsActive() const { return m_IsActive; }

    protected:
        virtual void OnMount() {}
        virtual void OnUnmount() {}
        virtual void OnEnable() {}
        virtual void OnDisable() {}
        virtual void OnUpdate() {}

    private:
        bool m_IsActive = true; // C++ 侧 ReadOnly，不能写该属性
    };

    // C++ 侧提供给 C# 侧的接口，不要用它
    class ComponentInternalUtility
    {
    public:
        static void SetIsActive(Component* component, bool value) { component->m_IsActive = value; }
        static void InvokeOnMount(Component* component) { component->OnMount(); }
        static void InvokeOnUnmount(Component* component) { component->OnUnmount(); }
        static void InvokeOnEnable(Component* component) { component->OnEnable(); }
        static void InvokeOnDisable(Component* component) { component->OnDisable(); }
        static void InvokeOnUpdate(Component* component) { component->OnUpdate(); }
    };
}
