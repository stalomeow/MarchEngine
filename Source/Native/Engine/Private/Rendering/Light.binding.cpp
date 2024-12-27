#include "pch.h"
#include "Engine/Rendering/Light.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO Light_New()
{
    retcs DBG_NEW Light();
}

NATIVE_EXPORT_AUTO Light_Delete(cs<Light*> pLight)
{
    delete pLight;
}

NATIVE_EXPORT_AUTO Light_GetType(cs<Light*> pLight)
{
    retcs pLight->Type;
}

NATIVE_EXPORT_AUTO Light_SetType(cs<Light*> pLight, cs<LightType> type)
{
    pLight->Type = type;
}

NATIVE_EXPORT_AUTO Light_GetColor(cs<Light*> pLight)
{
    retcs pLight->Color;
}

NATIVE_EXPORT_AUTO Light_SetColor(cs<Light*> pLight, cs_color color)
{
    pLight->Color = color;
}

NATIVE_EXPORT_AUTO Light_GetFalloffRange(cs<Light*> pLight)
{
    retcs pLight->FalloffRange;
}

NATIVE_EXPORT_AUTO Light_SetFalloffRange(cs<Light*> pLight, cs_vec2 range)
{
    pLight->FalloffRange = range;
}

NATIVE_EXPORT_AUTO Light_GetSpotPower(cs<Light*> pLight)
{
    retcs pLight->SpotPower;
}

NATIVE_EXPORT_AUTO Light_SetSpotPower(cs<Light*> pLight, cs_float power)
{
    pLight->SpotPower = power;
}
