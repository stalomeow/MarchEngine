#include "pch.h"
#include "Engine/Component.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO Component_NewDefault()
{
    retcs MARCH_NEW Component();
}

NATIVE_EXPORT_AUTO Component_GetIsActiveAndEnabled(cs<Component*> component)
{
    retcs component->GetIsActiveAndEnabled();
}

NATIVE_EXPORT_AUTO Component_SetIsActiveAndEnabled(cs<Component*> component, cs_bool value)
{
    ComponentInternalUtility::SetIsActiveAndEnabled(component, value);
}

NATIVE_EXPORT_AUTO Component_SetTransform(cs<Component*> component, cs<Transform*> value)
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

NATIVE_EXPORT_AUTO Component_OnDrawGizmos(cs<Component*> component, cs_bool isSelected)
{
    ComponentInternalUtility::InvokeOnDrawGizmos(component, isSelected);
}

NATIVE_EXPORT_AUTO Component_OnDrawGizmosGUI(cs<Component*> component, cs_bool isSelected)
{
    ComponentInternalUtility::InvokeOnDrawGizmosGUI(component, isSelected);
}
