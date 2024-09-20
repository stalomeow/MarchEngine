#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <string>
#include <queue>
#include <vector>
#include <stdint.h>

namespace march
{
    class GfxDevice;
    class GfxResource;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    enum class GfxCommandListType
    {
        Graphics,
        Compute,
        Copy,
        NumCommandList,
    };

    class GfxCommandList
    {
    public:
        GfxCommandList(GfxDevice* device, GfxCommandListType type, const std::string& name);
        ~GfxCommandList() = default;

        void Begin(ID3D12CommandAllocator* commandAllocator, uint32_t descriptorHeapCount, ID3D12DescriptorHeap* const* descriptorHeaps);
        void End();

        void ResourceBarrier(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter, bool immediate = false);
        void FlushResourceBarriers();

        GfxDevice* GetDevice() const { return m_Device; }
        GfxCommandListType GetType() const { return m_Type; }
        ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_CommandList.Get(); }

    private:
        GfxDevice* m_Device;
        GfxCommandListType m_Type;
        std::string m_Name;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

        std::vector<CD3DX12_RESOURCE_BARRIER> m_ResourceBarriers;

    public:
        static GfxCommandListType FromD3D12Type(D3D12_COMMAND_LIST_TYPE type);
        static D3D12_COMMAND_LIST_TYPE ToD3D12Type(GfxCommandListType type);
    };

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
