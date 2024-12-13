#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <optional>

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

    struct GfxOfflineDescriptor : public D3D12_CPU_DESCRIPTOR_HANDLE
    {
    public:
        GfxOfflineDescriptor() = default;
        GfxOfflineDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle) : D3D12_CPU_DESCRIPTOR_HANDLE(handle), m_Version(0) {}
        operator bool() const { return ptr != 0; }

        // D3D12_CPU_DESCRIPTOR_HANDLE 本质上是一个指针，用 version 表示指针指向的内容的版本
        // 当内容变化时，增加 version，使以前的缓存失效
        void IncrementVersion() { m_Version++; }
        uint32_t GetVersion() const { return m_Version; }

        bool operator==(const GfxOfflineDescriptor& other) const
        {
            return ptr == other.ptr && m_Version == other.m_Version;
        }

    private:
        uint32_t m_Version;
    };

    class GfxOfflineDescriptorAllocator
    {
    public:
        GfxOfflineDescriptorAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t pageSize);

        GfxOfflineDescriptor Allocate();
        void Release(const GfxOfflineDescriptor& descriptor);

        GfxDevice* GetDevice() const { return m_Device; }
        D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return m_Type; }
        uint32_t GetPageSize() const { return m_PageSize; }

    private:
        GfxDevice* m_Device;
        const D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
        const uint32_t m_PageSize;

        uint32_t m_NextDescriptorIndex;
        std::vector<std::unique_ptr<GfxDescriptorHeap>> m_Pages;
        std::queue<std::pair<uint64_t, GfxOfflineDescriptor>> m_ReleaseQueue;
    };

    template <size_t Capacity>
    class GfxOfflineDescriptorTable
    {
    public:
        GfxOfflineDescriptorTable() = default;

        // 当 root signature 变化时，全部清空
        // 如果 root signature 没变，可以通过设置 dirty 位来标记 descriptor table 是否需要更新，没变则可以不重新设置 root descriptor table
        void Reset()
        {
            m_Count = 0;
            m_IsDirty = false;
        }

        bool IsDirty() const
        {
            return m_IsDirty;
        }

        // 当切换 heap 时，需要设置为 dirty
        // 当设置 root descriptor table 后，需要清除 dirty 标记
        void SetDirty(bool value)
        {
            m_IsDirty = value;
        }

        size_t GetCount() const
        {
            return m_Count;
        }

        constexpr size_t GetCapacity() const
        {
            return Capacity;
        }

        const GfxOfflineDescriptor& Get(size_t index) const
        {
            return m_Descriptors[index];
        }

        void Set(size_t index, const GfxOfflineDescriptor& descriptor)
        {
            if (index < m_Count && m_Descriptors[index] == descriptor)
            {
                return;
            }

            m_Count = std::max(m_Count, index + 1);
            m_Descriptors[index] = descriptor;
            m_IsDirty = true;
        }

    private:
        size_t m_Count; // 设置的最大 index + 1
        GfxOfflineDescriptor m_Descriptors[Capacity];
        bool m_IsDirty;
    };

    struct GfxOnlineDescriptorTable : public D3D12_GPU_DESCRIPTOR_HANDLE
    {
    public:
        GfxOnlineDescriptorTable(D3D12_GPU_DESCRIPTOR_HANDLE handle, uint32_t numDescriptors)
            : D3D12_GPU_DESCRIPTOR_HANDLE(handle), m_NumDescriptors(numDescriptors) {}

        uint32_t GetNumDescriptors() const { return m_NumDescriptors; }

    private:
        uint32_t m_NumDescriptors;
    };

    class GfxOnlineViewDescriptorTableAllocator
    {
    public:
        GfxOnlineViewDescriptorTableAllocator(GfxDevice* device, uint32_t numMaxDescriptors);

        std::optional<GfxOnlineDescriptorTable> AllocateOneFrame(
            const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors,
            uint32_t numDescriptors);

        void CleanUpAllocations();

        uint32_t GetFront() const { return m_Front; }
        uint32_t GetRear() const { return m_Rear; }
        uint32_t GetNumMaxDescriptors() const { return m_NumMaxDescriptors; }
        GfxDescriptorHeap* GetHeap() const { return m_Heap.get(); }

    private:
        // Ring buffer
        std::unique_ptr<GfxDescriptorHeap> m_Heap;
        uint32_t m_Front;
        uint32_t m_Rear;
        uint32_t m_NumMaxDescriptors;

        std::queue<std::pair<uint64_t, uint32_t>> m_ReleaseQueue;
    };

    class GfxOnlineSamplerDescriptorTableAllocator
    {

    };
}
