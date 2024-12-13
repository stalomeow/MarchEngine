#include "GfxDescriptor.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include "Debug.h"
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
        if (!m_ReleaseQueue.empty() && m_Device->IsFrameFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            GfxOfflineDescriptor result = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();

            // 增加版号，使以前的缓存失效
            result.IncrementVersion();
            return result;
        }

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

        return m_Pages.back()->GetCpuHandle(m_NextDescriptorIndex++);
    }

    void GfxOfflineDescriptorAllocator::Release(const GfxOfflineDescriptor& descriptor)
    {
        m_ReleaseQueue.emplace(m_Device->GetNextFrameFence(), descriptor);
    }

    GfxOnlineViewDescriptorTableAllocator::GfxOnlineViewDescriptorTableAllocator(GfxDevice* device, uint32_t numMaxDescriptors)
        : m_Front(0)
        , m_Rear(0)
        , m_NumMaxDescriptors(numMaxDescriptors)
        , m_ReleaseQueue{}
    {
        GfxDescriptorHeapDesc heapDesc{};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.Capacity = numMaxDescriptors;
        heapDesc.ShaderVisible = true;

        m_Heap = std::make_unique<GfxDescriptorHeap>(device, "OnlineViewDescriptorTableRingBuffer", heapDesc);
    }

    std::optional<GfxOnlineDescriptorTable> GfxOnlineViewDescriptorTableAllocator::AllocateOneFrame(const D3D12_CPU_DESCRIPTOR_HANDLE* offlineDescriptors, uint32_t numDescriptors)
    {
        // 循环队列需要保留一个空间来区分队列满和队列空
        if (numDescriptors > m_NumMaxDescriptors - 1)
        {
            return std::nullopt;
        }

        bool canAllocate = false;

        if (m_Front <= m_Rear)
        {
            uint32_t remaining = m_NumMaxDescriptors - m_Rear;

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

        m_Rear = (m_Rear + numDescriptors) % m_NumMaxDescriptors;
        return GfxOnlineDescriptorTable{ handle, numDescriptors };
    }

    void GfxOnlineViewDescriptorTableAllocator::CleanUpAllocations()
    {
        GfxDevice* device = m_Heap->GetDevice();

        while (!m_ReleaseQueue.empty() && device->IsFrameFenceCompleted(m_ReleaseQueue.front().first, /* useCache */ true))
        {
            m_Front = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();
        }

        m_ReleaseQueue.emplace(device->GetNextFrameFence(), m_Rear);
    }
}
