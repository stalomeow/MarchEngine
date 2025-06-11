#pragma once

#include "Engine/Memory/Allocator.h"
#include <d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <list>
#include <unordered_map>
#include <functional>

namespace march
{
    class GfxDevice;

    struct GfxDescriptorHeapDesc
    {
        D3D12_DESCRIPTOR_HEAP_TYPE Type;
        uint32_t Capacity;
        bool ShaderVisible;
    };

    class GfxDescriptorHeap
    {
    public:
        GfxDescriptorHeap(GfxDevice* device, const std::string& name, const GfxDescriptorHeapDesc& desc);

        D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle(uint32_t index) const;
        D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(uint32_t index) const;
        void CopyFrom(const D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptors, uint32_t numDescriptors, uint32_t destStartIndex) const;

        GfxDevice* GetDevice() const { return m_Device; }
        ID3D12DescriptorHeap* GetD3DDescriptorHeap() const { return m_Heap.Get(); }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Heap->GetDesc().Type; }
        uint32_t GetCapacity() const { return static_cast<uint32_t>(m_Heap->GetDesc().NumDescriptors); }
        uint32_t GetIncrementSize() const { return m_IncrementSize; }

        bool IsShaderVisible() const
        {
            const D3D12_DESCRIPTOR_HEAP_FLAGS shaderVisibleFlag = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            const D3D12_DESCRIPTOR_HEAP_DESC desc = m_Heap->GetDesc();
            return (desc.Flags & shaderVisibleFlag) == shaderVisibleFlag;
        }

    private:
        GfxDevice* m_Device;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
        uint32_t m_IncrementSize;
    };

    class GfxOfflineDescriptorAllocator
    {
        friend class GfxOfflineDescriptor;

    public:
        GfxOfflineDescriptorAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t pageSize);

        GfxOfflineDescriptor Allocate();

        GfxDevice* GetDevice() const { return m_Device; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }
        uint32_t GetPageSize() const { return m_PageSize; }

    private:
        GfxDevice* m_Device;
        const D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
        const uint32_t m_PageSize;

        uint32_t m_NextDescriptorIndex;
        std::vector<std::unique_ptr<GfxDescriptorHeap>> m_Pages;
        std::queue<std::pair<uint64_t, D3D12_CPU_DESCRIPTOR_HANDLE>> m_ReleaseQueue;

        void DeferredRelease(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    };

    class GfxOfflineDescriptor final
    {
    public:
        GfxOfflineDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, GfxOfflineDescriptorAllocator* allocator);
        GfxOfflineDescriptor() : GfxOfflineDescriptor({}, nullptr) {}

        void DeferredRelease();
        ~GfxOfflineDescriptor() { DeferredRelease(); }

        D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const { return m_Handle; }
        operator bool() const { return m_Allocator != nullptr && m_Handle.ptr != 0; }

        GfxOfflineDescriptor(const GfxOfflineDescriptor&) = delete;
        GfxOfflineDescriptor& operator=(const GfxOfflineDescriptor&) = delete;

        GfxOfflineDescriptor(GfxOfflineDescriptor&&) noexcept;
        GfxOfflineDescriptor& operator=(GfxOfflineDescriptor&&);

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_Handle;
        GfxOfflineDescriptorAllocator* m_Allocator;
    };

    class GfxOnlineDescriptorAllocator
    {
    public:
        virtual ~GfxOnlineDescriptorAllocator() = default;

        // 分配结果只有一帧有效
        virtual bool AllocateMany(
            size_t numAllocations,
            const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
            const uint32_t* numDescriptors,
            D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults) = 0;
        virtual void CleanUpAllocations() = 0;

        virtual uint32_t GetNumMaxDescriptors() const = 0;
        virtual uint32_t GetNumAllocatedDescriptors() const = 0;
        virtual GfxDescriptorHeap* GetHeap() const = 0;
    };

    class GfxOnlineViewDescriptorAllocator : public GfxOnlineDescriptorAllocator
    {
    public:
        GfxOnlineViewDescriptorAllocator(GfxDevice* device, uint32_t numMaxDescriptors);

        bool AllocateMany(
            size_t numAllocations,
            const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
            const uint32_t* numDescriptors,
            D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults) override;
        void CleanUpAllocations() override;

        uint32_t GetNumMaxDescriptors() const override { return m_Heap->GetCapacity(); }
        uint32_t GetNumAllocatedDescriptors() const override { return (m_Rear + m_Heap->GetCapacity() - m_Front) % m_Heap->GetCapacity(); }
        GfxDescriptorHeap* GetHeap() const override { return m_Heap.get(); }

        uint32_t GetFront() const { return m_Front; }
        uint32_t GetRear() const { return m_Rear; }

    private:
        // Ring buffer
        std::unique_ptr<GfxDescriptorHeap> m_Heap;
        uint32_t m_Front;
        uint32_t m_Rear;

        std::queue<std::pair<uint64_t, uint32_t>> m_ReleaseQueue;
    };

    class GfxOnlineSamplerDescriptorAllocator : public GfxOnlineDescriptorAllocator
    {
    public:
        GfxOnlineSamplerDescriptorAllocator(GfxDevice* device, uint32_t numMaxDescriptors);

        bool AllocateMany(
            size_t numAllocations,
            const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
            const uint32_t* numDescriptors,
            D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults) override;
        void CleanUpAllocations() override;

        uint32_t GetNumMaxDescriptors() const override { return m_Allocator.GetMaxSize(); }
        uint32_t GetNumAllocatedDescriptors() const override { return m_Allocator.GetTotalAllocatedSize(); }
        GfxDescriptorHeap* GetHeap() const override { return m_Heap.get(); }

    private:
        std::unique_ptr<GfxDescriptorHeap> m_Heap;
        BuddyAllocator m_Allocator;

        std::list<size_t> m_Blocks; // 保存 Hash，越往后越旧

        struct BlockData
        {
            uint64_t Fence{};
            decltype(m_Blocks)::iterator Iterator{};

            uint32_t Offset{}; // Offset in heap
            D3D12_GPU_DESCRIPTOR_HANDLE Handle{};
            BuddyAllocation Allocation{};
        };

        std::unordered_map<size_t, BlockData> m_BlockMap; // Hash 和对应的 block
    };

    class GfxOnlineDescriptorMultiAllocator final
    {
    public:
        using Factory = typename std::function<std::unique_ptr<GfxOnlineDescriptorAllocator>(GfxDevice*)>;

        GfxOnlineDescriptorMultiAllocator(GfxDevice* device, const Factory& factory);

        // 分配结果只有一帧有效
        bool AllocateMany(
            size_t numAllocations,
            const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
            const uint32_t* numDescriptors,
            D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults,
            GfxDescriptorHeap** ppOutHeap);

        void CleanUpAllocations();

        void Rollover();

        GfxOnlineDescriptorAllocator* GetCurrentAllocator() const { return m_CurrentAllocator.get(); }
        GfxDevice* GetDevice() const { return m_Device; }

    private:
        GfxDevice* m_Device;
        Factory m_Factory;
        std::unique_ptr<GfxOnlineDescriptorAllocator> m_CurrentAllocator;
        std::queue<std::pair<uint64_t, std::unique_ptr<GfxOnlineDescriptorAllocator>>> m_ReleaseQueue;
    };
}
