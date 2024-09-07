#pragma once

#include <DirectXMath.h>

namespace dx12demo
{
    struct LightData
    {
        static const int MaxCount = 16;

        DirectX::XMFLOAT4 Position;      // 位置（w==1, 点光源/聚光灯）；方向（w==0, 平行光）
        DirectX::XMFLOAT4 SpotDirection; // 聚光灯方向（w 为聚光强度，0 表示非聚光灯）
        DirectX::XMFLOAT4 Color;         // 颜色，w 未使用
        DirectX::XMFLOAT4 Falloff;       // 衰减的起始和结束距离（点光源/聚光灯），zw 未使用
    };

    enum class LightType
    {
        Directional = 0, // 平行光
        Point = 1,       // 点光源
        Spot = 2,        // 聚光灯
    };

    class Light
    {
    public:
        DirectX::XMFLOAT3 GetForward() const
        {
            DirectX::XMFLOAT3 forward = { 0.0f, 0.0f, 1.0f };
            DirectX::XMVECTOR f = DirectX::XMLoadFloat3(&forward);
            f = DirectX::XMVector3Rotate(f, DirectX::XMLoadFloat4(&Rotation));
            DirectX::XMStoreFloat3(&forward, f);
            return forward;
        }

        void FillLightData(LightData& data) const
        {
            if (Type == LightType::Directional)
            {
                DirectX::XMFLOAT3 forward = GetForward();
                data.Position.x = -forward.x;
                data.Position.y = -forward.y;
                data.Position.z = -forward.z;
                data.Position.w = 0;
            }
            else
            {
                data.Position.x = Position.x;
                data.Position.y = Position.y;
                data.Position.z = Position.z;
                data.Position.w = 1;
            }

            if (Type == LightType::Spot)
            {
                DirectX::XMFLOAT3 forward = GetForward();
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

        DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
        DirectX::XMFLOAT4 Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
        bool IsActive = false;

        LightType Type = LightType::Directional;
        DirectX::XMFLOAT4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT2 FalloffRange = { 1.0f, 10.0f };
        float SpotPower = 64.0f;
    };
}
