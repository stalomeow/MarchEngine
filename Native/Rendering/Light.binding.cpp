#include "Rendering/Light.h"
#include "Scripting/ScriptTypes.h"

using namespace dx12demo;

NATIVE_EXPORT(Light*) Light_New()
{
    return new Light();
}

NATIVE_EXPORT(void) Light_Delete(Light* pLight)
{
    delete pLight;
}

NATIVE_EXPORT(void) Light_SetPosition(Light* pLight, CSharpVector3 position)
{
    pLight->Position = ToXMFLOAT3(position);
}

NATIVE_EXPORT(void) Light_SetRotation(Light* pLight, CSharpQuaternion rotation)
{
    pLight->Rotation = ToXMFLOAT4(rotation);
}

NATIVE_EXPORT(void) Light_SetIsActive(Light* pLight, CSharpBool isActive)
{
    pLight->IsActive = isActive;
}

NATIVE_EXPORT(CSharpInt) Light_GetType(Light* pLight)
{
    return static_cast<CSharpInt>(pLight->Type);
}

NATIVE_EXPORT(void) Light_SetType(Light* pLight, CSharpInt type)
{
    pLight->Type = static_cast<LightType>(type);
}

NATIVE_EXPORT(CSharpColor) Light_GetColor(Light* pLight)
{
    return ToCSharpColor(pLight->Color);
}

NATIVE_EXPORT(void) Light_SetColor(Light* pLight, CSharpColor color)
{
    pLight->Color = ToXMFLOAT4(color);
}

NATIVE_EXPORT(CSharpVector2) Light_GetFalloffRange(Light* pLight)
{
    return ToCSharpVector2(pLight->FalloffRange);
}

NATIVE_EXPORT(void) Light_SetFalloffRange(Light* pLight, CSharpVector2 range)
{
    pLight->FalloffRange = ToXMFLOAT2(range);
}

NATIVE_EXPORT(CSharpFloat) Light_GetSpotPower(Light* pLight)
{
    return pLight->SpotPower;
}

NATIVE_EXPORT(void) Light_SetSpotPower(Light* pLight, CSharpFloat power)
{
    pLight->SpotPower = power;
}
