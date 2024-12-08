#pragma once

#include "GfxDeviceChild.h"
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
    class GfxCommandQueue;
    class GfxCommandContext;

    class GfxFence : public GfxDeviceChild
    {
        friend GfxCommandQueue;

    public:
        GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue = 0);
        ~GfxFence();

        uint64_t GetCompletedValue() const;
        bool IsCompleted(uint64_t value) const;

        void Wait() const;
        void Wait(uint64_t value) const;
        uint64_t SignalNextValue();
        uint64_t GetNextValue() const { return m_Value + 1; }

    private:
        Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
        HANDLE m_EventHandle;
        uint64_t m_Value; // 上一次 signal 的值，可能从 cpu 侧 signal，也可能从 gpu 侧 signal
    };

    class GfxSyncPoint final
    {
        friend GfxCommandQueue;

    public:
        void Wait() const;
        bool IsCompleted() const;

        GfxSyncPoint(const GfxSyncPoint&) = default;
        GfxSyncPoint& operator=(const GfxSyncPoint&) = default;

        GfxSyncPoint(GfxSyncPoint&&) = default;
        GfxSyncPoint& operator=(GfxSyncPoint&&) = default;

    private:
        GfxSyncPoint(GfxFence* fence, uint64_t value) : m_Fence(fence), m_Value(value) {}

        GfxFence* m_Fence;
        uint64_t m_Value;
    };

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    enum class GfxCommandType
    {
        Direct, // 3D rendering engine
        Compute,
        Copy,
        NumTypes,
    };

    class GfxCommandQueue : public GfxDeviceChild
    {
        template <typename T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

    public:
        GfxCommandQueue(GfxDevice* device, const std::string& name, GfxCommandType type, int32_t priority = 0, bool disableGpuTimeout = false);

        GfxCommandType GetType() const { return m_Type; }
        ID3D12CommandQueue* GetQueue() const { return m_Queue.Get(); }

        GfxSyncPoint CreateSyncPoint();
        void Wait(const GfxSyncPoint& syncPoint);

        GfxCommandContext* GetContext();
        void SubmitContext(GfxCommandContext* context);

    private:
        GfxCommandType m_Type;
        ComPtr<ID3D12CommandQueue> m_Queue;

        std::vector<ComPtr<ID3D12CommandAllocator>> m_Allocators; // 保存所有的 allocator 的引用
        std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_FreeQueue;
    };

    class GfxCommandContext
    {
    public:
        GfxCommandContext(GfxDevice* device, GfxCommandListType type, const std::string& name);

        void Begin(ID3D12CommandAllocator* commandAllocator, uint32_t descriptorHeapCount, ID3D12DescriptorHeap* const* descriptorHeaps);
        void End();

        void ResourceBarrier(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter, bool immediate = false);
        void AddResourceBarrier(const CD3DX12_RESOURCE_BARRIER& resourceBarrier);
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
    };
}
