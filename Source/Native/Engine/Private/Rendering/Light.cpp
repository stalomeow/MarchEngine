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
        else
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
        m_Color = value;
    }

    float Light::GetIntensity() const
    {
        return m_Intensity;
    }

    void Light::SetIntensity(float value)
    {
        if (value < 0)
        {
            LOG_ERROR("Intensity must be greater than or equal to 0.");
            return;
        }

        m_Intensity = value;
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
        if (value < 0)
        {
            LOG_ERROR("Attenuation radius must be greater than or equal to 0.");
            return;
        }

        m_AttenuationRadius = value;
    }

    float Light::GetSpotInnerConeAngle() const
    {
        return m_SpotInnerConeAngle;
    }

    void Light::SetSpotInnerConeAngle(float value)
    {
        if (value < 0 || value > m_SpotOuterConeAngle)
        {
            LOG_ERROR("Inner cone angle must be in the range [0, outer cone angle].");
            return;
        }

        m_SpotInnerConeAngle = value;
    }

    float Light::GetSpotOuterConeAngle() const
    {
        return m_SpotOuterConeAngle;
    }

    void Light::SetSpotOuterConeAngle(float value)
    {
        if (value < m_SpotInnerConeAngle || value > 90.0f)
        {
            LOG_ERROR("Outer cone angle must be in the range [inner cone angle, 90].");
            return;
        }

        m_SpotOuterConeAngle = value;
    }

    void Light::FillLightData(LightData& data) const
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
        data.Color = GfxUtils::GetShaderColor(m_Color);
        data.Color.x *= intensity;
        data.Color.y *= intensity;
        data.Color.z *= intensity;

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

    static float GetSpotLightSolidAngle(float spotAngle)
    {
        float rad = XMConvertToRadians(spotAngle);
        return 2.0f * XM_PI * (1.0f - cosf(rad * 0.5f));
    }

    static constexpr float LumenToCandela(float lumen, float solidAngle)
    {
        return lumen / solidAngle;
    }

    static constexpr float CandelaToLumen(float candela, float solidAngle)
    {
        return candela * solidAngle;
    }

    bool LightUnitUtils::ConvertIntensity(LightType lightType, LightUnit from, LightUnit to, float& intensity, float spotAngle)
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
            solidAngle = GetSpotLightSolidAngle(spotAngle);
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

    float LightUnitUtils::GetIntensityForShader(LightType lightType, LightUnit unit, float intensity, float spotAngle)
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
                solidAngle = GetSpotLightSolidAngle(spotAngle);
                break;
            default:
                throw std::runtime_error("Unsupported light type.");
            }

            return LumenToCandela(intensity, solidAngle);
        }

        return intensity;
    }
}
