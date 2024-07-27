#pragma once

#include "Rendering/Resource/GpuBuffer.h"
#include <d3d12.h>
#include <wrl.h>
#include <DirectXMath.h>
#include <memory>

namespace dx12demo
{
    struct PerObjConstants
    {
        DirectX::XMFLOAT4X4 WorldMatrix;
    };

    struct PerDrawConstants
    {
        DirectX::XMFLOAT4X4 ViewMatrix;
        DirectX::XMFLOAT4X4 ProjectionMatrix;
        DirectX::XMFLOAT4X4 ViewProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewMatrix;
        DirectX::XMFLOAT4X4 InvProjectionMatrix;
        DirectX::XMFLOAT4X4 InvViewProjectionMatrix;
        DirectX::XMFLOAT4 Time; // elapsed time, delta time, unused, unused
    };

    class FrameResource
    {
    public:
        FrameResource(ID3D12Device* device, UINT64 fence, UINT objectCount, UINT drawCount);
        FrameResource(const FrameResource& rhs) = delete;
        FrameResource& operator=(const FrameResource& rhs) = delete;
        ~FrameResource() = default;

    public:
        UINT64 FenceValue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandAllocator;
        std::unique_ptr<ConstantBuffer<PerObjConstants>> PerObjectConstBuffer;
        std::unique_ptr<ConstantBuffer<PerDrawConstants>> PerDrawConstBuffer;
    };
}
