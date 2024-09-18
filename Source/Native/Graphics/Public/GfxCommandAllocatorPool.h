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

        GfxCommandAllocatorPool(const GfxCommandAllocatorPool&) = delete;
        GfxCommandAllocatorPool& operator=(const GfxCommandAllocatorPool&) = delete;

        void BeginFrame();
        void EndFrame(uint64_t fenceValue);
        ID3D12CommandAllocator* Get();

        GfxCommandListType GetType() const { return m_Type; }

    private:
        GfxDevice* m_Device;
        GfxCommandListType m_Type;

        std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_Allocators; // 保存所有的 allocator 的引用
        std::vector<ID3D12CommandAllocator*> m_UsedAllocators;
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReleaseQueue;
    };
}
