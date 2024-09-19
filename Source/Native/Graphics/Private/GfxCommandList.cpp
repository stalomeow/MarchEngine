#include "GfxCommandList.h"
#include "GfxDevice.h"
#include "GfxResource.h"
#include "GfxExcept.h"
#include "StringUtility.h"
#include <assert.h>

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
            m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(pRes, stateBefore, stateAfter));
            resource->SetState(stateAfter);
        }

        if (immediate)
        {
            FlushResourceBarriers();
        }
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
}
