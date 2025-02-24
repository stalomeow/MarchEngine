#pragma once

#include "Engine/Component.h"
#include <DirectXMath.h>

namespace march
{
    struct LightData
    {
        static const int MaxCount = 16;

        DirectX::XMFLOAT4 Position;      // 位置（w==1, 点光源/聚光灯）；方向（w==0, 平行光）
        DirectX::XMFLOAT4 SpotDirection; // 聚光灯方向，w 未使用
        DirectX::XMFLOAT4 Color;         // 颜色，接收阴影（w==1）
        DirectX::XMFLOAT4 Params;        // AttenuationRadius, cos(SpotOuterConeAngle), rcp(cos(SpotInnerConeAngle)-cos(SpotOuterConeAngle)), IsSpotLight (w==1)
    };

    enum class LightType
    {
        Directional,
        Point,
        Spot,
    };

    enum class LightUnit
    {
        Lux,
        Lumen,
        Candela,
    };

    class Light : public Component
    {
        static constexpr float DefaultDirectionalIntensity = DirectX::XM_PI;
        static constexpr LightUnit DefaultDirectionalUnit = LightUnit::Lux;

        static constexpr float DefaultPunctualIntensity = 8.0f;
        static constexpr LightUnit DefaultPunctualUnit = LightUnit::Candela;

        LightType m_Type = LightType::Directional;

        DirectX::XMFLOAT4 m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };

        float m_Intensity = DefaultDirectionalIntensity;
        LightUnit m_Unit = DefaultDirectionalUnit;

        float m_AttenuationRadius = 10.0f;

        // Note: 代码中的 Cone Angle 是圆锥顶角的一半，使用角度制

        float m_SpotInnerConeAngle = 0.0f;  // in degrees
        float m_SpotOuterConeAngle = 45.0f; // in degrees

        bool m_UseColorTemperature = false;
        float m_ColorTemperature = 6500.0f; // in Kelvin, Default to D65

    public:
        LightType GetType() const;
        void SetType(LightType value);

        const DirectX::XMFLOAT4& GetColor() const;
        void SetColor(const DirectX::XMFLOAT4& value);

        float GetIntensity() const;
        void SetIntensity(float value);

        LightUnit GetUnit() const;
        void SetUnit(LightUnit value);

        float GetAttenuationRadius() const;
        void SetAttenuationRadius(float value);

        float GetSpotInnerConeAngle() const;
        void SetSpotInnerConeAngle(float value);

        float GetSpotOuterConeAngle() const;
        void SetSpotOuterConeAngle(float value);

        bool GetUseColorTemperature() const;
        void SetUseColorTemperature(bool value);

        float GetColorTemperature() const;
        void SetColorTemperature(float value);

        void FillLightData(LightData& data, bool receiveShadow) const;
    };

    struct LightUnitUtils
    {
        static bool ConvertIntensity(LightType lightType, LightUnit from, LightUnit to, float& intensity, float spotAngle);
        static float GetIntensityForShader(LightType lightType, LightUnit unit, float intensity, float spotAngle);
    };
}
