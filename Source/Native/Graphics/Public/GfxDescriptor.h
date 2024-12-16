#pragma once

#include "Allocator.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <optional>
#include <list>
#include <unordered_map>
#include <functional>

#undef min
#undef max

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
        ~GfxDescriptorHeap();

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

        void Release(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    };

    class GfxOfflineDescriptor final
    {
    public:
        GfxOfflineDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, GfxOfflineDescriptorAllocator* allocator);
        GfxOfflineDescriptor() : GfxOfflineDescriptor({}, nullptr) {}

        ~GfxOfflineDescriptor() { Release(); }

        D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const { return m_Handle; }
        operator bool() const { return m_Allocator != nullptr && m_Handle.ptr != 0; }

        GfxOfflineDescriptor(const GfxOfflineDescriptor&) = delete;
        GfxOfflineDescriptor& operator=(const GfxOfflineDescriptor&) = delete;

        GfxOfflineDescriptor(GfxOfflineDescriptor&&);
        GfxOfflineDescriptor& operator=(GfxOfflineDescriptor&&);

    private:
        D3D12_CPU_DESCRIPTOR_HANDLE m_Handle;
        GfxOfflineDescriptorAllocator* m_Allocator;

        void Release();
    };

    template <size_t Capacity>
    class GfxOfflineDescriptorTable
    {
    public:
        GfxOfflineDescriptorTable() = default;

        void Reset()
        {
            m_Num = 0;
            memset(m_Descriptors, 0, sizeof(m_Descriptors));
            m_IsDirty = false;
        }

        void Set(size_t index, D3D12_CPU_DESCRIPTOR_HANDLE handle)
        {
            if (index < m_Num && m_Descriptors[index].ptr == handle.ptr)
            {
                return;
            }

            m_Num = std::max(m_Num, index + 1);
            m_Descriptors[index] = descriptor;
            m_IsDirty = true;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE* GetDescriptors() const { return m_Descriptors; }
        size_t GetNum() const { return m_Num; }
        bool IsEmpty() const { return m_Num == 0; }
        constexpr size_t GetCapacity() const { return Capacity; }
        bool IsDirty() const { return m_IsDirty; }
        void SetDirty(bool value) { m_IsDirty = value; }

    private:
        size_t m_Num; // 设置的最大 index + 1
        D3D12_CPU_DESCRIPTOR_HANDLE m_Descriptors[Capacity];
        bool m_IsDirty;
    };

    class GfxOnlineDescriptorTableAllocator
    {
    public:
        virtual ~GfxOnlineDescriptorTableAllocator() = default;

        // 分配结果只有一帧有效
        virtual std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> Allocate(
            const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors,
            uint32_t numDescriptors) = 0;
        virtual void CleanUpAllocations() = 0;

        virtual uint32_t GetNumMaxDescriptors() const = 0;
        virtual GfxDescriptorHeap* GetHeap() const = 0;
    };

    class GfxOnlineViewDescriptorTableAllocator : public GfxOnlineDescriptorTableAllocator
    {
    public:
        GfxOnlineViewDescriptorTableAllocator(GfxDevice* device, uint32_t numMaxDescriptors);

        std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> Allocate(
            const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors,
            uint32_t numDescriptors) override;
        void CleanUpAllocations() override;

        uint32_t GetNumMaxDescriptors() const override { return m_Heap->GetCapacity(); }
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

    class GfxOnlineSamplerDescriptorTableAllocator : public GfxOnlineDescriptorTableAllocator
    {
    public:
        GfxOnlineSamplerDescriptorTableAllocator(GfxDevice* device, uint32_t numMaxDescriptors);

        std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> Allocate(
            const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors,
            uint32_t numDescriptors) override;
        void CleanUpAllocations() override;

        uint32_t GetNumMaxDescriptors() const override { return m_Heap->GetCapacity(); }
        GfxDescriptorHeap* GetHeap() const override { return m_Heap.get(); }

    private:
        std::unique_ptr<GfxDescriptorHeap> m_Heap;
        BuddyAllocator m_Allocator;

        std::list<size_t> m_Blocks; // 保存 Hash，越往后越旧

        struct BlockData
        {
            uint64_t Fence;
            decltype(m_Blocks)::iterator Iterator;

            D3D12_GPU_DESCRIPTOR_HANDLE Handle;
            BuddyAllocation Allocation;
        };

        std::unordered_map<size_t, BlockData> m_BlockMap; // Hash 和对应的 block
    };

    class GfxOnlineDescriptorTableMultiAllocator final
    {
    public:
        using Factory = typename std::function<std::unique_ptr<GfxOnlineDescriptorTableAllocator>(GfxDevice*)>;

        GfxOnlineDescriptorTableMultiAllocator(GfxDevice* device, const Factory& factory);

        // 分配结果只有一帧有效
        D3D12_GPU_DESCRIPTOR_HANDLE Allocate(
            const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors,
            uint32_t numDescriptors,
            GfxDescriptorHeap** ppOutHeap);
        void CleanUpAllocations();

        GfxDevice* GetDevice() const { return m_Device; }

    private:
        GfxDevice* m_Device;
        Factory m_Factory;
        std::vector<std::unique_ptr<GfxOnlineDescriptorTableAllocator>> m_Allocators;
        std::queue<std::pair<uint64_t, GfxOnlineDescriptorTableAllocator*>> m_ReleaseQueue;
        GfxOnlineDescriptorTableAllocator* m_CurrentAllocator;

        void Rollover();
    };
}
