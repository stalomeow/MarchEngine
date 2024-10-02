#include "Light.h"
#include "Transform.h"
#include "RenderPipeline.h"
#include "WinApplication.h"

using namespace DirectX;

namespace march
{
    Light::Light()
        : Type(LightType::Directional)
        , Color(1.0f, 1.0f, 1.0f, 1.0f)
        , FalloffRange(1.0f, 10.0f)
        , SpotPower(64.0f)
    {
    }

    XMFLOAT3 Light::GetForward() const
    {
        XMFLOAT3 forward = { 0.0f, 0.0f, 1.0f };
        XMVECTOR f = XMLoadFloat3(&forward);
        f = XMVector3Rotate(f, GetTransform()->LoadRotation());
        XMStoreFloat3(&forward, f);
        return forward;
    }

    void Light::FillLightData(LightData& data) const
    {
        if (Type == LightType::Directional)
        {
            XMFLOAT3 forward = GetForward();
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

        if (Type == LightType::Spot)
        {
            XMFLOAT3 forward = GetForward();
            data.SpotDirection.x = -forward.x;
            data.SpotDirection.y = -forward.y;
            data.SpotDirection.z = -forward.z;
            data.SpotDirection.w = SpotPower;
        }
        else
        {
            data.SpotDirection.x = 0;
            data.SpotDirection.y = 0;
            data.SpotDirection.z = 0;
            data.SpotDirection.w = 0;
        }

        data.Color.x = Color.x;
        data.Color.y = Color.y;
        data.Color.z = Color.z;
        data.Color.w = 1;

        data.Falloff.x = FalloffRange.x;
        data.Falloff.y = FalloffRange.y;
        data.Falloff.z = 0;
        data.Falloff.w = 0;
    }

    void Light::OnMount()
    {
        Component::OnMount();
        GetApp().GetEngine()->GetRenderPipeline()->AddLight(this);
    }

    void Light::OnUnmount()
    {
        GetApp().GetEngine()->GetRenderPipeline()->RemoveLight(this);
        Component::OnUnmount();
    }
}
