#include "pch.h"
#include "Engine/Rendering/Light.h"
#include "Engine/Scripting/InteropServices.h"

NATIVE_EXPORT_AUTO Light_New()
{
    retcs MARCH_NEW Light();
}

NATIVE_EXPORT_AUTO Light_GetType(cs<Light*> pLight)
{
    retcs pLight->GetType();
}

NATIVE_EXPORT_AUTO Light_SetType(cs<Light*> pLight, cs<LightType> type)
{
    pLight->SetType(type);
}

NATIVE_EXPORT_AUTO Light_GetColor(cs<Light*> pLight)
{
    retcs pLight->GetColor();
}

NATIVE_EXPORT_AUTO Light_SetColor(cs<Light*> pLight, cs_color color)
{
    pLight->SetColor(color);
}

NATIVE_EXPORT_AUTO Light_GetIntensity(cs<Light*> pLight)
{
    retcs pLight->GetIntensity();
}

NATIVE_EXPORT_AUTO Light_SetIntensity(cs<Light*> pLight, cs_float intensity)
{
    pLight->SetIntensity(intensity);
}

NATIVE_EXPORT_AUTO Light_GetUnit(cs<Light*> pLight)
{
    retcs pLight->GetUnit();
}

NATIVE_EXPORT_AUTO Light_SetUnit(cs<Light*> pLight, cs<LightUnit> unit)
{
    pLight->SetUnit(unit);
}

NATIVE_EXPORT_AUTO Light_GetAttenuationRadius(cs<Light*> pLight)
{
    retcs pLight->GetAttenuationRadius();
}

NATIVE_EXPORT_AUTO Light_SetAttenuationRadius(cs<Light*> pLight, cs_float radius)
{
    pLight->SetAttenuationRadius(radius);
}

NATIVE_EXPORT_AUTO Light_GetSpotInnerConeAngle(cs<Light*> pLight)
{
    retcs pLight->GetSpotInnerConeAngle();
}

NATIVE_EXPORT_AUTO Light_SetSpotInnerConeAngle(cs<Light*> pLight, cs_float angle)
{
    pLight->SetSpotInnerConeAngle(angle);
}

NATIVE_EXPORT_AUTO Light_GetSpotOuterConeAngle(cs<Light*> pLight)
{
    retcs pLight->GetSpotOuterConeAngle();
}

NATIVE_EXPORT_AUTO Light_SetSpotOuterConeAngle(cs<Light*> pLight, cs_float angle)
{
    pLight->SetSpotOuterConeAngle(angle);
}

NATIVE_EXPORT_AUTO Light_GetUseColorTemperature(cs<Light*> pLight)
{
    retcs pLight->GetUseColorTemperature();
}

NATIVE_EXPORT_AUTO Light_SetUseColorTemperature(cs<Light*> pLight, cs_bool value)
{
    pLight->SetUseColorTemperature(value);
}

NATIVE_EXPORT_AUTO Light_GetColorTemperature(cs<Light*> pLight)
{
    retcs pLight->GetColorTemperature();
}

NATIVE_EXPORT_AUTO Light_SetColorTemperature(cs<Light*> pLight, cs_float temperature)
{
    pLight->SetColorTemperature(temperature);
}

NATIVE_EXPORT_AUTO Light_GetAngularDiameter(cs<Light*> pLight)
{
    retcs pLight->GetAngularDiameter();
}

NATIVE_EXPORT_AUTO Light_SetAngularDiameter(cs<Light*> pLight, cs_float value)
{
    pLight->SetAngularDiameter(value);
}

NATIVE_EXPORT_AUTO Light_GetShadowDepthBias(cs<Light*> pLight)
{
    retcs pLight->GetShadowDepthBias();
}

NATIVE_EXPORT_AUTO Light_SetShadowDepthBias(cs<Light*> pLight, cs_int value)
{
    pLight->SetShadowDepthBias(value);
}

NATIVE_EXPORT_AUTO Light_GetShadowSlopeScaledDepthBias(cs<Light*> pLight)
{
    retcs pLight->GetShadowSlopeScaledDepthBias();
}

NATIVE_EXPORT_AUTO Light_SetShadowSlopeScaledDepthBias(cs<Light*> pLight, cs_float value)
{
    pLight->SetShadowSlopeScaledDepthBias(value);
}

NATIVE_EXPORT_AUTO Light_GetShadowDepthBiasClamp(cs<Light*> pLight)
{
    retcs pLight->GetShadowDepthBiasClamp();
}

NATIVE_EXPORT_AUTO Light_SetShadowDepthBiasClamp(cs<Light*> pLight, cs_float value)
{
    pLight->SetShadowDepthBiasClamp(value);
}

NATIVE_EXPORT_AUTO Light_GetIsCastingShadow(cs<Light*> pLight)
{
    retcs pLight->GetIsCastingShadow();
}

NATIVE_EXPORT_AUTO Light_SetIsCastingShadow(cs<Light*> pLight, cs_bool value)
{
    pLight->SetIsCastingShadow(value);
}
