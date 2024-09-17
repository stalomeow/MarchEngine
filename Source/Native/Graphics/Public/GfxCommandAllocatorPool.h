#pragma once

#include <directx/d3dx12.h>
#include <queue>
#include <vector>
#include <stdint.h>
#include <wrl.h>

namespace march
{
    class GfxDevice;
    enum class GfxCommandListType;

    class GfxCommandAllocatorPool
    {
    public:
        GfxCommandAllocatorPool(GfxDevice* device, GfxCommandListType type);
        ~GfxCommandAllocatorPool() = default;

        ID3D12CommandAllocator* Get();
        void Release(ID3D12CommandAllocator* allocator);

    private:
        GfxDevice* m_Device;
        std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_Allocators;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReleaseQueue;
    };
}
