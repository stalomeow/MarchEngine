#include "GfxCommandList.h"
#include "GfxDevice.h"
#include "GfxResource.h"
#include "GfxExcept.h"
#include "GfxFence.h"
#include "StringUtility.h"
#include <assert.h>

using namespace Microsoft::WRL;

namespace march
{
    GfxCommandList::GfxCommandList(GfxDevice* device, GfxCommandListType type, const std::string& name)
        : m_Device(device), m_Type(type), m_Name(name), m_CommandList(nullptr), m_ResourceBarriers{}
    {
    }

    void GfxCommandList::Begin(ID3D12CommandAllocator* commandAllocator, uint32_t descriptorHeapCount, ID3D12DescriptorHeap* const* descriptorHeaps)
    {
        if (m_CommandList == nullptr)
        {
            ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();
            D3D12_COMMAND_LIST_TYPE type = ToD3D12Type(m_Type);
            GFX_HR(d3d12Device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(m_CommandList.GetAddressOf())));

#ifdef ENABLE_GFX_DEBUG_NAME
            GFX_HR(m_CommandList->SetName(StringUtility::Utf8ToUtf16(m_Name).c_str()));
#endif
        }
        else
        {
            GFX_HR(m_CommandList->Reset(commandAllocator, nullptr));
        }

        m_CommandList->SetDescriptorHeaps(static_cast<UINT>(descriptorHeapCount), descriptorHeaps);
    }

    void GfxCommandList::End()
    {
        FlushResourceBarriers();
        GFX_HR(m_CommandList->Close());
    }

    void GfxCommandList::ResourceBarrier(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter, bool immediate)
    {
        if (resource->NeedStateTransition(stateAfter))
        {
            ID3D12Resource* pRes = resource->GetD3D12Resource();
            D3D12_RESOURCE_STATES stateBefore = resource->GetState();
            AddResourceBarrier(CD3DX12_RESOURCE_BARRIER::Transition(pRes, stateBefore, stateAfter));
            resource->SetState(stateAfter);
        }

        if (immediate)
        {
            FlushResourceBarriers();
        }
    }

    void GfxCommandList::AddResourceBarrier(const CD3DX12_RESOURCE_BARRIER& resourceBarrier)
    {
        m_ResourceBarriers.push_back(resourceBarrier);
    }

    void GfxCommandList::FlushResourceBarriers()
    {
        if (!m_ResourceBarriers.empty())
        {
            // 尽量合并后一起提交
            UINT count = static_cast<UINT>(m_ResourceBarriers.size());
            m_CommandList->ResourceBarrier(count, m_ResourceBarriers.data());
            m_ResourceBarriers.clear();
        }
    }

    GfxCommandListType GfxCommandList::FromD3D12Type(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return GfxCommandListType::Graphics;

        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return GfxCommandListType::Compute;

        case D3D12_COMMAND_LIST_TYPE_COPY:
            return GfxCommandListType::Copy;

        default:
            throw GfxException("Invalid command list type");
        }
    }

    D3D12_COMMAND_LIST_TYPE GfxCommandList::ToD3D12Type(GfxCommandListType type)
    {
        switch (type)
        {
        case march::GfxCommandListType::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;

        case march::GfxCommandListType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;

        case march::GfxCommandListType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;

        default:
            throw GfxException("Invalid command list type");
        }
    }

    GfxCommandAllocatorPool::GfxCommandAllocatorPool(GfxDevice* device, GfxCommandListType type)
        : m_Device(device), m_Type(type), m_Allocators{}, m_UsedAllocators{}, m_ReleaseQueue{}
    {

    }

    void GfxCommandAllocatorPool::BeginFrame()
    {

    }

    void GfxCommandAllocatorPool::EndFrame(uint64_t fenceValue)
    {
        for (ID3D12CommandAllocator* allocator : m_UsedAllocators)
        {
            m_ReleaseQueue.emplace(fenceValue, allocator);
        }

        m_UsedAllocators.clear();
    }

    ID3D12CommandAllocator* GfxCommandAllocatorPool::Get()
    {
        ID3D12CommandAllocator* result;

        if (!m_ReleaseQueue.empty() && m_Device->IsGraphicsFenceCompleted(m_ReleaseQueue.front().first))
        {
            result = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();

            // Reuse the memory associated with command recording.
            // We can only reset when the associated command lists have finished execution on the GPU.
            GFX_HR(result->Reset());
        }
        else
        {
            ComPtr<ID3D12CommandAllocator> allocator = nullptr;
            ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();
            D3D12_COMMAND_LIST_TYPE type = GfxCommandList::ToD3D12Type(m_Type);
            GFX_HR(d3d12Device->CreateCommandAllocator(type, IID_PPV_ARGS(allocator.GetAddressOf())));

            m_Allocators.push_back(allocator);
            result = allocator.Get();
        }

        m_UsedAllocators.push_back(result);
        return result;
    }
}
