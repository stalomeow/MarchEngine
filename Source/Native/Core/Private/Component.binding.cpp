#include "Component.h"
#include "InteropServices.h"

NATIVE_EXPORT_AUTO Component_CreateDefault()
{
    return to_cs(new Component());
}

NATIVE_EXPORT_AUTO Component_DeleteDefault(cs<Component*> component)
{
    delete component;
}

NATIVE_EXPORT_AUTO Component_GetIsActiveAndEnabled(cs<Component*> component)
{
    return to_cs(component->GetIsActiveAndEnabled());
}

NATIVE_EXPORT_AUTO Component_SetIsActiveAndEnabled(cs<Component*> component, cs_bool value)
{
    ComponentInternalUtility::SetIsActiveAndEnabled(component, value);
}

NATIVE_EXPORT_AUTO Component_SetTransform(cs<Component*> component, Transform* value)
{
    ComponentInternalUtility::SetTransform(component, value);
}

NATIVE_EXPORT_AUTO Component_OnMount(cs<Component*> component)
{
    ComponentInternalUtility::InvokeOnMount(component);
}

NATIVE_EXPORT_AUTO Component_OnUnmount(cs<Component*> component)
{
    ComponentInternalUtility::InvokeOnUnmount(component);
}

NATIVE_EXPORT_AUTO Component_OnEnable(cs<Component*> component)
{
    ComponentInternalUtility::InvokeOnEnable(component);
}

NATIVE_EXPORT_AUTO Component_OnDisable(cs<Component*> component)
{
    ComponentInternalUtility::InvokeOnDisable(component);
}

NATIVE_EXPORT_AUTO Component_OnUpdate(cs<Component*> component)
{
    ComponentInternalUtility::InvokeOnUpdate(component);
}
