#include "GfxCommand.h"
#include "GfxDevice.h"
#include "GfxResource.h"
#include <assert.h>

using namespace Microsoft::WRL;

namespace march
{
    GfxCommandContext::GfxCommandContext(GfxDevice* device, GfxCommandType type)
        : m_Device(device)
        , m_Type(type)
        , m_CommandAllocator(nullptr)
        , m_CommandList(nullptr)
        , m_ResourceBarriers{}
        , m_SyncPointsToWait{}
    {
    }

    void GfxCommandContext::Open()
    {
        assert(m_CommandAllocator == nullptr);

        GfxCommandQueue* queue = m_Device->GetCommandManager()->GetQueue(m_Type);
        m_CommandAllocator = queue->RequestCommandAllocator();

        if (m_CommandList == nullptr)
        {
            GFX_HR(m_Device->GetD3DDevice4()->CreateCommandList(0, queue->GetType(),
                m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
        }
        else
        {
            GFX_HR(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
        }
    }

    GfxSyncPoint GfxCommandContext::SubmitAndRelease()
    {
        GfxCommandManager* manager = m_Device->GetCommandManager();
        GfxCommandQueue* queue = manager->GetQueue(m_Type);

        // 准备好所有命令，然后关闭
        FlushResourceBarriers();
        GFX_HR(m_CommandList->Close());

        // 等待其他 queue 上的异步操作，例如 async compute, async copy
        for (const GfxSyncPoint& syncPoint : m_SyncPointsToWait)
        {
            queue->WaitOnGpu(syncPoint);
        }

        // 正式提交
        ID3D12CommandList* commandLists[] = { m_CommandList.Get() };
        queue->GetQueue()->ExecuteCommandLists(static_cast<UINT>(std::size(commandLists)), commandLists);
        GfxSyncPoint syncPoint = queue->ReleaseCommandAllocator(m_CommandAllocator);

        // 释放临时资源
        m_CommandAllocator = nullptr;
        m_ResourceBarriers.clear();
        m_SyncPointsToWait.clear();

        // 回收
        manager->RecycleContext(this);
        return syncPoint;
    }

    void GfxCommandContext::TransitionResource(GfxResource* resource, D3D12_RESOURCE_STATES stateAfter)
    {
        D3D12_RESOURCE_STATES stateBefore = resource->GetState();
        bool needTransition;

        if (stateAfter == D3D12_RESOURCE_STATE_COMMON)
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states
            // D3D12_RESOURCE_STATE_COMMON 为 0，要特殊处理
            needTransition = stateBefore != stateAfter;
        }
        else
        {
            needTransition = (stateBefore & stateAfter) != stateAfter;
        }

        if (needTransition)
        {
            ID3D12Resource* res = resource->GetD3DResource();
            m_ResourceBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(res, stateBefore, stateAfter));
            resource->SetState(stateAfter);
        }
    }

    void GfxCommandContext::FlushResourceBarriers()
    {
        // 尽量合并后一起提交

        if (!m_ResourceBarriers.empty())
        {
            UINT count = static_cast<UINT>(m_ResourceBarriers.size());
            m_CommandList->ResourceBarrier(count, m_ResourceBarriers.data());
            m_ResourceBarriers.clear();
        }
    }

    void GfxCommandContext::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        m_SyncPointsToWait.push_back(syncPoint);
    }
}
