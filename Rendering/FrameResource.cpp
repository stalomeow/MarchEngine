#include "Rendering/FrameResouce.h"

namespace dx12demo
{
    FrameResource::FrameResource(ID3D12Device* device, UINT64 fence, UINT objectCount, UINT drawCount)
        : FenceValue(fence)
    {
        PerObjectConstBuffer = std::make_unique<ConstantBuffer<PerObjConstants>>(
            L"FrameRes::PerObjConstantBuffer", objectCount);
        PerDrawConstBuffer = std::make_unique<ConstantBuffer<PerDrawConstants>>(
            L"FrameRes::PerDrawConstantBuffer", drawCount);
    }
}
