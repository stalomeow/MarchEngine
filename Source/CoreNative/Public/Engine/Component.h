#pragma once

#include "Engine/Object.h"

namespace march
{
    class Transform;

    class Component : public MarchObject
    {
        friend struct ComponentInternalUtility;

    public:
        virtual bool GetIsActiveAndEnabled() const { return m_IsActiveAndEnabled; }
        Transform* GetTransform() const { return m_Transform; }

    protected:
        virtual void OnMount() {}
        virtual void OnUnmount() {}
        virtual void OnEnable() {}
        virtual void OnDisable() {}
        virtual void OnUpdate() {}

    private:
        bool m_IsActiveAndEnabled = false;
        Transform* m_Transform = nullptr;
    };

    // C++ 侧提供给 C# 侧的接口，不要用它
    struct ComponentInternalUtility
    {
        static void SetIsActiveAndEnabled(Component* component, bool value) { component->m_IsActiveAndEnabled = value; }
        static void SetTransform(Component* component, Transform* value) { component->m_Transform = value; }
        static void InvokeOnMount(Component* component) { component->OnMount(); }
        static void InvokeOnUnmount(Component* component) { component->OnUnmount(); }
        static void InvokeOnEnable(Component* component) { component->OnEnable(); }
        static void InvokeOnDisable(Component* component) { component->OnDisable(); }
        static void InvokeOnUpdate(Component* component) { component->OnUpdate(); }
    };
}
