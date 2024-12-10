#include "GfxCommand.h"
#include "GfxDevice.h"
#include "GfxUtils.h"
#include <Windows.h>

namespace march
{
    GfxCommandQueue::GfxCommandQueue(GfxDevice* device, const std::string& name, GfxCommandType type, int32_t priority, bool disableGpuTimeout)
        : m_Type(type)
        , m_Allocators{}
        , m_FreeAllocators{}
    {
        D3D12_COMMAND_QUEUE_DESC desc{};
        desc.Priority = priority;

        switch (type)
        {
        case GfxCommandType::Direct:
            desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;
        case GfxCommandType::Compute:
            desc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;
        case GfxCommandType::Copy:
            desc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            break;
        default:
            throw GfxException("Invalid command type.");
        }

        if (disableGpuTimeout)
        {
            desc.Flags |= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        }

        GFX_HR(device->GetD3DDevice4()->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_Queue)));
        GfxUtils::SetName(m_Queue.Get(), name);

        m_Fence = std::make_unique<GfxFence>(device, name + " Fence");
    }

    GfxSyncPoint GfxCommandQueue::CreateSyncPoint()
    {
        uint64_t v = m_Fence->SignalNextValue([this](ID3D12Fence* fence, uint64_t value)
        {
            GFX_HR(m_Queue->Signal(fence, static_cast<UINT64>(value)));
        });
        return GfxSyncPoint{ m_Fence.get(), v };
    }

    void GfxCommandQueue::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        ID3D12Fence* fence = syncPoint.GetFence()->GetFence();
        GFX_HR(m_Queue->Wait(fence, static_cast<UINT64>(syncPoint.GetValue())));
    }

    void GfxCommandQueue::SubmitContext(GfxCommandContext* context)
    {
        if (context->GetType() != GetType())
        {
            throw GfxException("Command context type mismatch.");
        }

        ID3D12CommandList* list = context->GetList();
        m_Queue->ExecuteCommandLists(1, &list);
    }
}
