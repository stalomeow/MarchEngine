#include "GfxDescriptor.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include "Debug.h"
#include <Windows.h>

namespace march
{
    static LPCSTR GetDescriptorHeapTypeName(D3D12_DESCRIPTOR_HEAP_TYPE type)
    {
        switch (type)
        {
        case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
            return "CBV_SRV_UAV";
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
        m_Device->DeferredRelease(m_Heap);
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

    GfxDescriptorHandle::GfxDescriptorHandle(GfxDescriptorHeap* heap, uint32_t pageIndex, uint32_t heapIndex)
        : m_Heap(heap), m_PageIndex(pageIndex), m_HeapIndex(heapIndex)
    {
    }

    GfxDescriptorAllocator::GfxDescriptorAllocator(GfxDevice* device, D3D12_DESCRIPTOR_HEAP_TYPE descriptorType)
        : m_Device(device), m_DescriptorType(descriptorType)
        , m_NextDescriptorIndex(0), m_Pages{}, m_FrameFreeList{}, m_ReleaseQueue{}
    {
    }

    void GfxDescriptorAllocator::BeginFrame()
    {
    }

    void GfxDescriptorAllocator::EndFrame(uint64_t fenceValue)
    {
        for (const GfxDescriptorHandle& handle : m_FrameFreeList)
        {
            m_ReleaseQueue.emplace(fenceValue, handle);
        }

        m_FrameFreeList.clear();
    }

    GfxDescriptorHandle GfxDescriptorAllocator::Allocate()
    {
        if (!m_ReleaseQueue.empty() && m_Device->IsGraphicsFenceCompleted(m_ReleaseQueue.front().first))
        {
            GfxDescriptorHandle result = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();
            return result;
        }

        if (m_Pages.empty() || m_NextDescriptorIndex >= PageSize)
        {
            m_NextDescriptorIndex = 0;

            std::string name = "GfxDescriptorPage" + std::to_string(m_Pages.size());
            m_Pages.push_back(std::make_unique<GfxDescriptorHeap>(m_Device, m_DescriptorType, PageSize, false, name));
            LOG_TRACE("Create %s; Size: %d; Type: %s", name.c_str(), PageSize, GetDescriptorHeapTypeName(m_DescriptorType));
        }

        return GfxDescriptorHandle(m_Pages.back().get(), static_cast<uint32_t>(m_Pages.size()) - 1, m_NextDescriptorIndex++);
    }

    void GfxDescriptorAllocator::Free(const GfxDescriptorHandle& handle)
    {
        // TODO check if the handle is valid
        m_FrameFreeList.push_back(handle);
    }

    GfxDescriptorTable::GfxDescriptorTable(GfxDescriptorHeap* heap, uint32_t offset, uint32_t count)
        : m_Heap(heap), m_Offset(offset), m_Count(count)
    {
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxDescriptorTable::GetCpuHandle(uint32_t index) const
    {
        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor table");
        }

        return m_Heap->GetCpuHandle(m_Offset + index);
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GfxDescriptorTable::GetGpuHandle(uint32_t index) const
    {
        if (index >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor table");
        }

        return m_Heap->GetGpuHandle(m_Offset + index);
    }

    void GfxDescriptorTable::Copy(uint32_t destIndex, D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor) const
    {
        if (destIndex >= m_Count)
        {
            throw std::out_of_range("Index out of the range of descriptor heap span");
        }

        m_Heap->Copy(m_Offset + destIndex, srcDescriptor);
    }

    GfxDescriptorTableAllocator::GfxDescriptorTableAllocator(GfxDevice* device, GfxDescriptorTableType type, uint32_t staticDescriptorCount, uint32_t dynamicDescriptorCapacity)
        : m_ReleaseQueue{}, m_DynamicFront(0), m_DynamicRear(0), m_DynamicCapacity(dynamicDescriptorCapacity)
    {
        D3D12_DESCRIPTOR_HEAP_TYPE heapType = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(type);
        std::string name = GetDescriptorHeapTypeName(heapType) + std::string("_DescriptorTablePool");
        uint32_t capacity = dynamicDescriptorCapacity + staticDescriptorCount; // static 部分放在 dynamic 后面
        m_Heap = std::make_unique<GfxDescriptorHeap>(device, heapType, capacity, true, name);
    }

    void GfxDescriptorTableAllocator::BeginFrame()
    {
        GfxDevice* device = m_Heap->GetDevice();

        while (!m_ReleaseQueue.empty() && device->IsGraphicsFenceCompleted(m_ReleaseQueue.front().first))
        {
            m_DynamicFront = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();
        }
    }

    void GfxDescriptorTableAllocator::EndFrame(uint64_t fenceValue)
    {
        m_ReleaseQueue.emplace(fenceValue, m_DynamicRear);
    }

    GfxDescriptorTable GfxDescriptorTableAllocator::AllocateDynamicTable(uint32_t descriptorCount)
    {
        // 循环队列需要保留一个空间来区分队列满和队列空
        if (descriptorCount > m_DynamicCapacity - 1)
        {
            throw std::out_of_range("Dynamic descriptor table size exceeds the capacity of the allocator");
        }

        bool canAllocate = false;

        if (m_DynamicFront <= m_DynamicRear)
        {
            UINT remaining = m_DynamicCapacity - m_DynamicRear;

            if (m_DynamicFront == 0)
            {
                // 空出一个位置来区分队列满和队列空
                if (remaining - 1 >= descriptorCount)
                {
                    canAllocate = true;
                }
            }
            else
            {
                if (remaining < descriptorCount)
                {
                    m_DynamicRear = 0; // 后面不够了，从头开始分配，之后 Front > Rear
                }
                else
                {
                    canAllocate = true;
                }
            }
        }

        if (m_DynamicFront - m_DynamicRear - 1 >= descriptorCount)
        {
            canAllocate = true;
        }

        if (!canAllocate)
        {
            throw std::out_of_range("Descriptor table pool is full");
        }

        GfxDescriptorTable table(m_Heap.get(), m_DynamicRear, descriptorCount);
        m_DynamicRear = (m_DynamicRear + descriptorCount) % m_DynamicCapacity;
        return table;
    }

    GfxDescriptorTable GfxDescriptorTableAllocator::GetStaticTable() const
    {
        return GfxDescriptorTable(m_Heap.get(), m_DynamicCapacity, GetStaticDescriptorCount());
    }
}
