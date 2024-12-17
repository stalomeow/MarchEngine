#include "GfxDescriptor.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include "Debug.h"
#include "HashUtils.h"
#include <Windows.h>
#include <stdexcept>

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

    GfxOfflineDescriptor::GfxOfflineDescriptor(GfxOfflineDescriptor&& other) noexcept
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

    GfxOnlineViewDescriptorAllocator::GfxOnlineViewDescriptorAllocator(GfxDevice* device, uint32_t numMaxDescriptors)
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

    bool GfxOnlineViewDescriptorAllocator::AllocateMany(
        size_t numAllocations,
        const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
        const uint32_t* numDescriptors,
        D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults)
    {
        constexpr size_t MAX_NUM_ALLOCATIONS = 20;
        if (numAllocations > MAX_NUM_ALLOCATIONS)
        {
            return false;
        }

        uint32_t numMaxDescriptors = m_Heap->GetCapacity();
        uint32_t totalNumDescriptors = 0;

        for (size_t i = 0; i < numAllocations; i++)
        {
            totalNumDescriptors += numDescriptors[i];
        }

        // 循环队列需要保留一个空间来区分队列满和队列空
        if (totalNumDescriptors > numMaxDescriptors - 1)
        {
            return false;
        }

        uint32_t initialRear = m_Rear; // 保存 Rear，以便回滚
        uint32_t indices[MAX_NUM_ALLOCATIONS]{};

        for (size_t i = 0; i < numAllocations; i++)
        {
            if (numDescriptors[i] == 0)
            {
                continue;
            }

            bool canAllocate = false;

            if (m_Front <= m_Rear)
            {
                uint32_t remaining = numMaxDescriptors - m_Rear;

                if (m_Front == 0)
                {
                    // 空出一个位置来区分队列满和队列空
                    if (remaining - 1 >= numDescriptors[i])
                    {
                        canAllocate = true;
                    }
                }
                else
                {
                    if (remaining < numDescriptors[i])
                    {
                        m_Rear = 0; // 后面不够了，从头开始分配，之后 Front > Rear
                    }
                    else
                    {
                        canAllocate = true;
                    }
                }
            }

            if (m_Front - m_Rear - 1 >= numDescriptors[i])
            {
                canAllocate = true;
            }

            if (!canAllocate)
            {
                m_Rear = initialRear; // 回滚
                return false;
            }

            indices[i] = m_Rear;
            m_Rear = (m_Rear + numDescriptors[i]) % numMaxDescriptors;
        }

        for (size_t i = 0; i < numAllocations; i++)
        {
            if (numDescriptors[i] == 0)
            {
                pOutResults[i].ptr = 0;
            }
            else
            {
                m_Heap->CopyFrom(offlineDescriptors[i], numDescriptors[i], indices[i]);
                pOutResults[i] = m_Heap->GetGpuHandle(indices[i]);
            }
        }

        return true;
    }

    void GfxOnlineViewDescriptorAllocator::CleanUpAllocations()
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

    GfxOnlineSamplerDescriptorAllocator::GfxOnlineSamplerDescriptorAllocator(GfxDevice* device, uint32_t numMaxDescriptors)
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

    bool GfxOnlineSamplerDescriptorAllocator::AllocateMany(
        size_t numAllocations,
        const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
        const uint32_t* numDescriptors,
        D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults)
    {
        constexpr size_t MAX_NUM_ALLOCATIONS = 20;
        if (numAllocations > MAX_NUM_ALLOCATIONS)
        {
            return false;
        }

        uint32_t numMaxDescriptors = m_Heap->GetCapacity();
        uint32_t totalNumDescriptors = 0;

        for (size_t i = 0; i < numAllocations; i++)
        {
            totalNumDescriptors += numDescriptors[i];
        }

        if (totalNumDescriptors > numMaxDescriptors)
        {
            return false;
        }

        size_t hashes[MAX_NUM_ALLOCATIONS]{};
        bool isNew[MAX_NUM_ALLOCATIONS]{};

        for (size_t i = 0; i < numAllocations; i++)
        {
            if (numDescriptors[i] == 0)
            {
                continue;
            }

            // sampler 会根据 hash 复用，所以这里也可以根据 hash 复用一组 sampler

            DefaultHash hash{};
            for (uint32_t j = 0; j < numDescriptors[i]; j++)
            {
                hash.Append(offlineDescriptors[i][j]);
            }
            hashes[i] = *hash;

            if (m_BlockMap.count(*hash) > 0)
            {
                isNew[i] = false;
            }
            else
            {
                isNew[i] = true;
                BlockData data{};

                if (std::optional<uint32_t> offset = m_Allocator.Allocate(numDescriptors[i], 0, data.Allocation))
                {
                    data.Offset = *offset;
                    data.Handle = m_Heap->GetGpuHandle(*offset);
                    m_BlockMap.emplace(*hash, data);
                }
                else
                {
                    // 分配失败，回滚
                    for (size_t j = 0; j < i; j++)
                    {
                        if (numDescriptors[i] > 0 && isNew[j])
                        {
                            m_Allocator.Release(m_BlockMap[hashes[j]].Allocation);
                            m_BlockMap.erase(hashes[j]);
                        }
                    }

                    return false;
                }
            }
        }

        uint64_t fence = m_Heap->GetDevice()->GetNextFrameFence();

        for (size_t i = 0; i < numAllocations; i++)
        {
            if (numDescriptors[i] == 0)
            {
                pOutResults[i].ptr = 0;
            }
            else
            {
                BlockData& data = m_BlockMap[hashes[i]];
                pOutResults[i] = data.Handle;

                if (isNew[i])
                {
                    m_Heap->CopyFrom(offlineDescriptors[i], numDescriptors[i], data.Offset);
                }
                else
                {
                    // 准备将 block 移动到最前面
                    m_Blocks.erase(data.Iterator);
                }

                data.Fence = fence;
                data.Iterator = m_Blocks.emplace(m_Blocks.begin(), hashes[i]); // 放在最前面
            }
        }

        return true;
    }

    void GfxOnlineSamplerDescriptorAllocator::CleanUpAllocations()
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

    GfxOnlineDescriptorMultiAllocator::GfxOnlineDescriptorMultiAllocator(GfxDevice* device, const Factory& factory)
        : m_Device(device)
        , m_Factory(factory)
        , m_Allocators{}
        , m_ReleaseQueue{}
        , m_CurrentAllocator(nullptr)
    {
        Rollover();
    }

    bool GfxOnlineDescriptorMultiAllocator::AllocateMany(
        size_t numAllocations,
        const D3D12_CPU_DESCRIPTOR_HANDLE* const* offlineDescriptors,
        const uint32_t* numDescriptors,
        D3D12_GPU_DESCRIPTOR_HANDLE* pOutResults,
        GfxDescriptorHeap** ppOutHeap)
    {
        if (m_CurrentAllocator->AllocateMany(numAllocations, offlineDescriptors, numDescriptors, pOutResults))
        {
            *ppOutHeap = m_CurrentAllocator->GetHeap();
            return true;
        }

        return false;
    }

    void GfxOnlineDescriptorMultiAllocator::CleanUpAllocations()
    {
        m_CurrentAllocator->CleanUpAllocations();
    }

    void GfxOnlineDescriptorMultiAllocator::Rollover()
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
