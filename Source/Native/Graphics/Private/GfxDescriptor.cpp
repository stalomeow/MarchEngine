#include "GfxDescriptor.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include "Debug.h"
#include "HashUtils.h"
#include <Windows.h>
#include <stdexcept>
#include <assert.h>

namespace march
{
    static std::string to_string(D3D12_DESCRIPTOR_HEAP_TYPE type)
    {
        switch (type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return "CBV/SRV/UAV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
            return "SAMPLER";
        case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
            return "RTV";
        case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
            return "DSV";
        default:
            return "UNKNOWN";
        }
    }

    GfxDescriptorHeap::GfxDescriptorHeap(GfxDevice* device, const std::string& name, const GfxDescriptorHeapDesc& desc)
        : m_Device(device)
        , m_Heap(nullptr)
    {
        ID3D12Device4* d3dDevice = device->GetD3DDevice4();

        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = desc.Type;
        heapDesc.NumDescriptors = static_cast<UINT>(desc.Capacity);
        heapDesc.Flags = desc.ShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        GFX_HR(d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap)));
        GfxUtils::SetName(m_Heap.Get(), name);

        m_IncrementSize = static_cast<uint32_t>(d3dDevice->GetDescriptorHandleIncrementSize(desc.Type));
    }

    GfxDescriptorHeap::~GfxDescriptorHeap()
    {
        if (m_Device && m_Heap)
        {
            m_Device->DeferredRelease(m_Heap);

            m_Device = nullptr;
            m_Heap = nullptr;
        }
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxDescriptorHeap::GetCpuHandle(uint32_t index) const
    {
        if (index >= GetCapacity())
        {
            throw std::out_of_range("GfxDescriptorHeap::GetCpuHandle: Index out of the range of descriptor heap");
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle{ m_Heap->GetCPUDescriptorHandleForHeapStart() };
        return handle.Offset(static_cast<INT>(index), static_cast<UINT>(m_IncrementSize));
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GfxDescriptorHeap::GetGpuHandle(uint32_t index) const
    {
        if (index >= GetCapacity())
        {
            throw std::out_of_range("GfxDescriptorHeap::GetGpuHandle: Index out of the range of descriptor heap");
        }

        CD3DX12_GPU_DESCRIPTOR_HANDLE handle{ m_Heap->GetGPUDescriptorHandleForHeapStart() };
        return handle.Offset(static_cast<INT>(index), static_cast<UINT>(m_IncrementSize));
    }

    void GfxDescriptorHeap::CopyFrom(const D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptors, uint32_t numDescriptors, uint32_t destStartIndex) const
    {
        if (destStartIndex + numDescriptors > GetCapacity())
        {
            throw std::out_of_range("GfxDescriptorHeap::Copy: Index out of the range of descriptor heap");
        }

        D3D12_CPU_DESCRIPTOR_HANDLE destDescriptors = GetCpuHandle(destStartIndex);
        UINT numCopy = static_cast<UINT>(numDescriptors);

        // pSrcDescriptorRangeSizes 为 nullptr 表示所有范围的大小均为 1
        m_Device->GetD3DDevice4()->CopyDescriptors(1, &destDescriptors, &numCopy, numCopy, srcDescriptors, nullptr, GetType());
    }

    GfxOfflineDescriptorAllocator::GfxOfflineDescriptorAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t pageSize)
        : m_Device(device)
        , m_Type(type)
        , m_PageSize(pageSize)
        , m_NextDescriptorIndex(0)
        , m_Pages{}
        , m_ReleaseQueue{}
    {
    }

    GfxOfflineDescriptor GfxOfflineDescriptorAllocator::Allocate()
    {
        D3D12_CPU_DESCRIPTOR_HANDLE handle;

        if (!m_ReleaseQueue.empty() && m_Device->IsFrameFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            handle = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();
        }
        else
        {
            if (m_Pages.empty() || m_NextDescriptorIndex >= m_PageSize)
            {
                std::string heapName = "GfxOfflineDescriptorPage" + std::to_string(m_Pages.size());

                GfxDescriptorHeapDesc heapDesc{};
                heapDesc.Type = m_Type;
                heapDesc.Capacity = m_PageSize;
                heapDesc.ShaderVisible = false;

                m_Pages.push_back(std::make_unique<GfxDescriptorHeap>(m_Device, heapName, heapDesc));
                m_NextDescriptorIndex = 0;

                LOG_TRACE("Create %s; Size: %u; Type: %s", heapName.c_str(), m_PageSize, to_string(m_Type).c_str());
            }

            handle = m_Pages.back()->GetCpuHandle(m_NextDescriptorIndex++);
        }

        return GfxOfflineDescriptor{ handle, this };
    }

    void GfxOfflineDescriptorAllocator::Release(D3D12_CPU_DESCRIPTOR_HANDLE handle)
    {
        m_ReleaseQueue.emplace(m_Device->GetNextFrameFence(), handle);
    }

    GfxOfflineDescriptor::GfxOfflineDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle, GfxOfflineDescriptorAllocator* allocator)
        : m_Handle(handle)
        , m_Allocator(allocator)
    {
    }

    void GfxOfflineDescriptor::Release()
    {
        if (m_Allocator)
        {
            m_Allocator->Release(m_Handle);
            m_Allocator = nullptr;
        }

        m_Handle.ptr = 0;
    }

    GfxOfflineDescriptor::GfxOfflineDescriptor(GfxOfflineDescriptor&& other)
        : m_Handle(std::exchange(other.m_Handle, {}))
        , m_Allocator(std::exchange(other.m_Allocator, nullptr))
    {
    }

    GfxOfflineDescriptor& GfxOfflineDescriptor::operator=(GfxOfflineDescriptor&& other)
    {
        if (this != &other)
        {
            Release();

            m_Handle = std::exchange(other.m_Handle, {});
            m_Allocator = std::exchange(other.m_Allocator, nullptr);
        }

        return *this;
    }

    GfxOnlineViewDescriptorTableAllocator::GfxOnlineViewDescriptorTableAllocator(GfxDevice* device, uint32_t numMaxDescriptors)
        : m_Front(0)
        , m_Rear(0)
        , m_ReleaseQueue{}
    {
        GfxDescriptorHeapDesc heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Capacity = numMaxDescriptors;
        heapDesc.ShaderVisible = true;

        m_Heap = std::make_unique<GfxDescriptorHeap>(device, "OnlineViewDescriptorTableRingBuffer", heapDesc);
    }

    std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> GfxOnlineViewDescriptorTableAllocator::Allocate(const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors, uint32_t numDescriptors)
    {
        uint32_t numMaxDescriptors = m_Heap->GetCapacity();

        // 循环队列需要保留一个空间来区分队列满和队列空
        if (numDescriptors > numMaxDescriptors - 1)
        {
            return std::nullopt;
        }

        bool canAllocate = false;

        if (m_Front <= m_Rear)
        {
            uint32_t remaining = numMaxDescriptors - m_Rear;

            if (m_Front == 0)
            {
                // 空出一个位置来区分队列满和队列空
                if (remaining - 1 >= numDescriptors)
                {
                    canAllocate = true;
                }
            }
            else
            {
                if (remaining < numDescriptors)
                {
                    m_Rear = 0; // 后面不够了，从头开始分配，之后 Front > Rear
                }
                else
                {
                    canAllocate = true;
                }
            }
        }

        if (m_Front - m_Rear - 1 >= numDescriptors)
        {
            canAllocate = true;
        }

        if (!canAllocate)
        {
            return std::nullopt;
        }

        m_Heap->CopyFrom(offlineDescriptors, numDescriptors, m_Rear);
        D3D12_GPU_DESCRIPTOR_HANDLE handle = m_Heap->GetGpuHandle(m_Rear);

        m_Rear = (m_Rear + numDescriptors) % numMaxDescriptors;
        return handle;
    }

    void GfxOnlineViewDescriptorTableAllocator::CleanUpAllocations()
    {
        GfxDevice* device = m_Heap->GetDevice();

        while (!m_ReleaseQueue.empty() && device->IsFrameFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            m_Front = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();
        }

        // 每帧回收一次
        m_ReleaseQueue.emplace(device->GetNextFrameFence(), m_Rear);
    }

    GfxOnlineSamplerDescriptorTableAllocator::GfxOnlineSamplerDescriptorTableAllocator(GfxDevice* device, uint32_t numMaxDescriptors)
        : m_Allocator(1, numMaxDescriptors)
        , m_Blocks{}
        , m_BlockMap{}
    {
        GfxDescriptorHeapDesc desc{};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.Capacity = numMaxDescriptors;
        desc.ShaderVisible = true;

        m_Heap = std::make_unique<GfxDescriptorHeap>(device, "OnlineSamplerDescriptorTableBlocks", desc);
    }

    std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> GfxOnlineSamplerDescriptorTableAllocator::Allocate(const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors, uint32_t numDescriptors)
    {
        if (numDescriptors > m_Heap->GetCapacity())
        {
            throw std::invalid_argument("GfxOnlineSamplerDescriptorTableAllocator::AllocateOneFrame: numDescriptors exceeds the heap size");
        }

        DefaultHash hash{};
        for (uint32_t i = 0; i < numDescriptors; i++)
        {
            hash.Append(offlineDescriptors[i].ptr);
        }

        auto it = m_BlockMap.find(*hash);
        if (it != m_BlockMap.end())
        {
            m_Blocks.erase(it->second.Iterator);
        }
        else
        {
            BlockData data{};

            if (std::optional<uint32_t> offset = m_Allocator.Allocate(numDescriptors, 0, data.Allocation))
            {
                m_Heap->CopyFrom(offlineDescriptors, numDescriptors, *offset);
                data.Handle = m_Heap->GetGpuHandle(*offset);
                it = m_BlockMap.emplace(*hash, data).first;
            }
            else
            {
                return std::nullopt;
            }
        }

        it->second.Fence = m_Heap->GetDevice()->GetNextFrameFence();
        it->second.Iterator = m_Blocks.emplace(m_Blocks.begin(), *hash);
        return it->second.Handle;
    }

    void GfxOnlineSamplerDescriptorTableAllocator::CleanUpAllocations()
    {
        GfxDevice* device = m_Heap->GetDevice();

        while (!m_Blocks.empty())
        {
            auto it = m_BlockMap.find(m_Blocks.back());

            if (!device->IsFrameFenceCompleted(it->second.Fence))
            {
                break;
            }

            m_Allocator.Release(it->second.Allocation);
            m_BlockMap.erase(it);
            m_Blocks.pop_back();
        }
    }

    GfxOnlineDescriptorTableMultiAllocator::GfxOnlineDescriptorTableMultiAllocator(GfxDevice* device, const Factory& factory)
        : m_Device(device)
        , m_Factory(factory)
        , m_Allocators{}
        , m_ReleaseQueue{}
        , m_CurrentAllocator(nullptr)
    {
        Rollover();
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GfxOnlineDescriptorTableMultiAllocator::Allocate(const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors, uint32_t numDescriptors, GfxDescriptorHeap** ppOutHeap)
    {
        std::optional<D3D12_GPU_DESCRIPTOR_HANDLE> result;
        bool rollover = false;

        while (!(result = m_CurrentAllocator->Allocate(offlineDescriptors, numDescriptors)))
        {
            // 只允许一次 Rollover
            assert(!rollover);

            Rollover();
            rollover = true;
        }

        *ppOutHeap = m_CurrentAllocator->GetHeap();
        return *result;
    }

    void GfxOnlineDescriptorTableMultiAllocator::CleanUpAllocations()
    {
        m_CurrentAllocator->CleanUpAllocations();
    }

    void GfxOnlineDescriptorTableMultiAllocator::Rollover()
    {
        if (m_CurrentAllocator != nullptr)
        {
            // 切换 descriptor heap 会有性能开销
            // Ref: https://learn.microsoft.com/en-us/windows/win32/direct3d12/shader-visible-descriptor-heaps

            LOG_WARNING("DescriptorHeapRollover detected! Type: %s", to_string(m_CurrentAllocator->GetHeap()->GetType()));
            m_ReleaseQueue.emplace(m_Device->GetNextFrameFence(), m_CurrentAllocator);
        }

        if (!m_ReleaseQueue.empty() && m_Device->IsFrameFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            m_CurrentAllocator = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();

            // 回收空间
            m_CurrentAllocator->CleanUpAllocations();
        }
        else
        {
            m_CurrentAllocator = m_Allocators.emplace_back(m_Factory(m_Device)).get();
        }
    }
}
