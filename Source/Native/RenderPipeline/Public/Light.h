#pragma once

#include "Component.h"
#include <DirectXMath.h>

namespace march
{
    class Transform;

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

    class Light : public Component
    {
    public:
        Light();
        void FillLightData(LightData& data) const;

        LightType Type;
        DirectX::XMFLOAT4 Color;
        DirectX::XMFLOAT2 FalloffRange;
        float SpotPower;
    };
}
