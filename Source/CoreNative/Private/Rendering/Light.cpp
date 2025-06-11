#include "pch.h"
#include "Engine/Rendering/Light.h"
#include "Engine/Transform.h"
#include "Engine/Debug.h"
#include "Engine/Rendering/D3D12.h"
#include <math.h>
#include <assert.h>
#include <stdexcept>

using namespace DirectX;

namespace march
{
    LightType Light::GetType() const
    {
        return m_Type;
    }

    void Light::SetType(LightType value)
    {
        if (m_Type == value)
        {
            return;
        }

        if (m_Type == LightType::Directional)
        {
            m_Intensity = DefaultPunctualIntensity;
            m_Unit = DefaultPunctualUnit;
        }
        else if (value == LightType::Directional)
        {
            m_Intensity = DefaultDirectionalIntensity;
            m_Unit = DefaultDirectionalUnit;
        }

        m_Type = value;
    }

    const XMFLOAT4& Light::GetColor() const
    {
        return m_Color;
    }

    void Light::SetColor(const XMFLOAT4& value)
    {
        if (m_UseColorTemperature)
        {
            LOG_ERROR("Cannot set color when using color temperature.");
            return;
        }

        m_Color = value;
    }

    float Light::GetIntensity() const
    {
        return m_Intensity;
    }

    void Light::SetIntensity(float value)
    {
        m_Intensity = std::max(value, 0.0f);
    }

    LightUnit Light::GetUnit() const
    {
        return m_Unit;
    }

    void Light::SetUnit(LightUnit value)
    {
        if (!LightUnitUtils::ConvertIntensity(m_Type, m_Unit, value, m_Intensity, m_SpotOuterConeAngle))
        {
            LOG_ERROR("The light unit is not supported by the light type.");
            return;
        }

        m_Unit = value;
    }

    float Light::GetAttenuationRadius() const
    {
        return m_AttenuationRadius;
    }

    void Light::SetAttenuationRadius(float value)
    {
        m_AttenuationRadius = std::max(value, 0.0f);
    }

    float Light::GetSpotInnerConeAngle() const
    {
        return m_SpotInnerConeAngle;
    }

    void Light::SetSpotInnerConeAngle(float value)
    {
        m_SpotInnerConeAngle = std::clamp(value, 0.0f, m_SpotOuterConeAngle);
    }

    float Light::GetSpotOuterConeAngle() const
    {
        return m_SpotOuterConeAngle;
    }

    void Light::SetSpotOuterConeAngle(float value)
    {
        m_SpotOuterConeAngle = std::clamp(value, m_SpotInnerConeAngle, 90.0f);
    }

    // Ref: https://github.com/Unity-Technologies/Graphics/blob/ffdd1e73164d4090f51b37e7634776c87eb7f6cc/com.unity.render-pipelines.high-definition/Runtime/Lighting/LightUtils.cs#L293
    // Given a correlated color temperature (in Kelvin), estimate the RGB equivalent. Curve fit error is max 0.008.
    // return color in linear RGB space
    static XMFLOAT4 CorrelatedColorTemperatureToRGB(float temperature)
    {
        float r, g, b;

        // Temperature must fall between 1000 and 40000 degrees
        // The fitting require to divide kelvin by 1000 (allow more precision)
        float kelvin = std::clamp(temperature, 1000.0f, 40000.0f) / 1000.0f;
        float kelvin2 = kelvin * kelvin;

        // Using 6570 as a pivot is an approximation, pivot point for red is around 6580 and for blue and green around 6560.
        // Calculate each color in turn (Note, clamp is not really necessary as all value belongs to [0..1] but can help for extremum).
        // Red
        r = kelvin < 6.570f ? 1.0f : std::clamp((1.35651f + 0.216422f * kelvin + 0.000633715f * kelvin2) / (-3.24223f + 0.918711f * kelvin), 0.0f, 1.0f);
        // Green
        g = kelvin < 6.570f ?
            std::clamp((-399.809f + 414.271f * kelvin + 111.543f * kelvin2) / (2779.24f + 164.143f * kelvin + 84.7356f * kelvin2), 0.0f, 1.0f) :
            std::clamp((1370.38f + 734.616f * kelvin + 0.689955f * kelvin2) / (-4625.69f + 1699.87f * kelvin), 0.0f, 1.0f);
        //Blue
        b = kelvin > 6.570f ? 1.0f : std::clamp((348.963f - 523.53f * kelvin + 183.62f * kelvin2) / (2848.82f - 214.52f * kelvin + 78.8614f * kelvin2), 0.0f, 1.0f);

        return XMFLOAT4{ r, g, b, 1.0f };
    }

    bool Light::GetUseColorTemperature() const
    {
        return m_UseColorTemperature;
    }

    void Light::SetUseColorTemperature(bool value)
    {
        if (m_UseColorTemperature != value)
        {
            if ((m_UseColorTemperature = value))
            {
                m_Color = CorrelatedColorTemperatureToRGB(m_ColorTemperature);
            }
            else
            {
                m_Color = { 1.0f, 1.0f, 1.0f, 1.0f };
            }
        }
    }

    float Light::GetColorTemperature() const
    {
        return m_ColorTemperature;
    }

    void Light::SetColorTemperature(float value)
    {
        value = std::clamp(value, 1000.0f, 40000.0f);

        if (m_ColorTemperature != value)
        {
            m_ColorTemperature = value;

            if (m_UseColorTemperature)
            {
                m_Color = CorrelatedColorTemperatureToRGB(m_ColorTemperature);
            }
        }
    }

    float Light::GetAngularDiameter() const
    {
        return m_AngularDiameter;
    }

    void Light::SetAngularDiameter(float value)
    {
        m_AngularDiameter = std::clamp(value, 0.0f, 90.0f);
    }

    int32 Light::GetShadowDepthBias() const
    {
        return m_ShadowDepthBias;
    }

    void Light::SetShadowDepthBias(int32 value)
    {
        m_ShadowDepthBias = value;
    }

    float Light::GetShadowSlopeScaledDepthBias() const
    {
        return m_ShadowSlopeScaledDepthBias;
    }

    void Light::SetShadowSlopeScaledDepthBias(float value)
    {
        m_ShadowSlopeScaledDepthBias = value;
    }

    float Light::GetShadowDepthBiasClamp() const
    {
        return m_ShadowDepthBiasClamp;
    }

    void Light::SetShadowDepthBiasClamp(float value)
    {
        m_ShadowDepthBiasClamp = value;
    }

    bool Light::GetIsCastingShadow() const
    {
        return m_IsCastingShadow;
    }

    void Light::SetIsCastingShadow(bool value)
    {
        m_IsCastingShadow = value;
    }

    void Light::FillLightData(LightData& data, bool castShadow) const
    {
        if (m_Type == LightType::Directional)
        {
            XMFLOAT3 forward = GetTransform()->GetForward();
            data.Position.x = -forward.x;
            data.Position.y = -forward.y;
            data.Position.z = -forward.z;
            data.Position.w = 0;
        }
        else
        {
            data.Position.x = GetTransform()->GetPosition().x;
            data.Position.y = GetTransform()->GetPosition().y;
            data.Position.z = GetTransform()->GetPosition().z;
            data.Position.w = 1;
        }

        if (m_Type == LightType::Spot)
        {
            XMFLOAT3 forward = GetTransform()->GetForward();
            data.SpotDirection.x = -forward.x;
            data.SpotDirection.y = -forward.y;
            data.SpotDirection.z = -forward.z;
            data.SpotDirection.w = 0;
        }
        else
        {
            data.SpotDirection.x = 0;
            data.SpotDirection.y = 0;
            data.SpotDirection.z = 0;
            data.SpotDirection.w = 0;
        }

        float intensity = LightUnitUtils::GetIntensityForShader(m_Type, m_Unit, m_Intensity, m_SpotOuterConeAngle);
        bool isSRGBColor = !m_UseColorTemperature; // CorrelatedColorTemperatureToRGB 返回的是线性颜色
        data.Color = GfxUtils::GetShaderColor(m_Color, isSRGBColor);
        data.Color.x *= intensity;
        data.Color.y *= intensity;
        data.Color.z *= intensity;
        data.Color.w = castShadow ? 1.0f : 0.0f;

        float cosSpotInnerConeAngle = cosf(XMConvertToRadians(m_SpotInnerConeAngle));
        float cosSpotOuterConeAngle = cosf(XMConvertToRadians(m_SpotOuterConeAngle));
        data.Params.x = m_AttenuationRadius;
        data.Params.y = cosSpotOuterConeAngle;
        data.Params.z = 1.0f / (cosSpotInnerConeAngle - cosSpotOuterConeAngle + FLT_EPSILON);
        data.Params.w = (m_Type == LightType::Spot) ? 1.0f : 0.0f;
    }

    static constexpr float GetPointLightSolidAngle()
    {
        return 4.0f * XM_PI;
    }

    static float GetSpotLightSolidAngle(float spotConeAngleHalf)
    {
        float rad = XMConvertToRadians(spotConeAngleHalf);
        return 2.0f * XM_PI * (1.0f - cosf(rad));
    }

    static constexpr float LumenToCandela(float lumen, float solidAngle)
    {
        return lumen / std::max(solidAngle, FLT_EPSILON);
    }

    static constexpr float CandelaToLumen(float candela, float solidAngle)
    {
        return candela * solidAngle;
    }

    bool LightUnitUtils::ConvertIntensity(LightType lightType, LightUnit from, LightUnit to, float& intensity, float spotConeAngleHalf)
    {
        if (lightType == LightType::Directional)
        {
            // Directional 只支持 Lux
            return from == LightUnit::Lux && to == LightUnit::Lux;
        }

        if (from == to)
        {
            // Punctual 只支持 Lumen 和 Candela
            return from == LightUnit::Lumen || from == LightUnit::Candela;
        }

        float solidAngle;

        switch (lightType)
        {
        case LightType::Point:
            solidAngle = GetPointLightSolidAngle();
            break;
        case LightType::Spot:
            solidAngle = GetSpotLightSolidAngle(spotConeAngleHalf);
            break;
        default:
            LOG_ERROR("Unsupported light type.");
            return false;
        }

        if (from == LightUnit::Lumen && to == LightUnit::Candela)
        {
            intensity = LumenToCandela(intensity, solidAngle);
            return true;
        }

        if (from == LightUnit::Candela && to == LightUnit::Lumen)
        {
            intensity = CandelaToLumen(intensity, solidAngle);
            return true;
        }

        return false;
    }

    float LightUnitUtils::GetIntensityForShader(LightType lightType, LightUnit unit, float intensity, float spotConeAngleHalf)
    {
        if (lightType == LightType::Directional)
        {
            // Directional 使用 Lux
            assert(unit == LightUnit::Lux);
            return intensity;
        }

        // Punctual 使用 Candela

        if (unit != LightUnit::Candela)
        {
            assert(unit == LightUnit::Lumen);

            float solidAngle;

            switch (lightType)
            {
            case LightType::Point:
                solidAngle = GetPointLightSolidAngle();
                break;
            case LightType::Spot:
                solidAngle = GetSpotLightSolidAngle(spotConeAngleHalf);
                break;
            default:
                throw std::runtime_error("Unsupported light type.");
            }

            return LumenToCandela(intensity, solidAngle);
        }

        return intensity;
    }
}
