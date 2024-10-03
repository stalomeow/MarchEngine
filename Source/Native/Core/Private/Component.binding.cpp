#include "Component.h"
#include "InteropServices.h"

using namespace march;

NATIVE_EXPORT(Component*) Component_CreateDefault()
{
    return new Component();
}

NATIVE_EXPORT(void) Component_DeleteDefault(Component* component)
{
    delete component;
}

NATIVE_EXPORT(CSharpBool) Component_GetIsActiveAndEnabled(Component* component)
{
    return CSHARP_MARSHAL_BOOL(component->GetIsActiveAndEnabled());
}

NATIVE_EXPORT(void) Component_SetIsActiveAndEnabled(Component* component, CSharpBool value)
{
    ComponentInternalUtility::SetIsActiveAndEnabled(component, CSHARP_UNMARSHAL_BOOL(value));
}

NATIVE_EXPORT(void) Component_SetTransform(Component* component, Transform* value)
{
    ComponentInternalUtility::SetTransform(component, value);
}

NATIVE_EXPORT(void) Component_OnMount(Component* component)
{
    ComponentInternalUtility::InvokeOnMount(component);
}

NATIVE_EXPORT(void) Component_OnUnmount(Component* component)
{
    ComponentInternalUtility::InvokeOnUnmount(component);
}

NATIVE_EXPORT(void) Component_OnEnable(Component* component)
{
    ComponentInternalUtility::InvokeOnEnable(component);
}

NATIVE_EXPORT(void) Component_OnDisable(Component* component)
{
    ComponentInternalUtility::InvokeOnDisable(component);
}

NATIVE_EXPORT(void) Component_OnUpdate(Component* component)
{
    ComponentInternalUtility::InvokeOnUpdate(component);
}
