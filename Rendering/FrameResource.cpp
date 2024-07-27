#include "Rendering/FrameResouce.h"

namespace dx12demo
{
    FrameResource::FrameResource(ID3D12Device* device, UINT64 fence, UINT objectCount, UINT drawCount)
        : FenceValue(fence)
    {
        THROW_IF_FAILED(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CommandAllocator)));

        PerObjectConstBuffer = std::make_unique<ConstantBuffer<PerObjConstants>>(
            L"FrameRes::PerObjConstantBuffer", objectCount);
        PerDrawConstBuffer = std::make_unique<ConstantBuffer<PerDrawConstants>>(
            L"FrameRes::PerDrawConstantBuffer", drawCount);
    }
}
