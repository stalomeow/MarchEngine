#include "GfxCommand.h"
#include "GfxDevice.h"
#include "GfxFence.h"
#include "StringUtils.h"
#include <Windows.h>

namespace march
{
    GfxCommandQueue::GfxCommandQueue(GfxDevice* device, GfxCommandListType type, const std::string& name, int32_t priority, bool disableGpuTimeout)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = GfxCommandList::ToD3D12Type(type);
        queueDesc.Priority = priority;

        if (disableGpuTimeout)
        {
            queueDesc.Flags |= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        }

        ID3D12Device4* d3d12Device = device->GetD3D12Device();
        GFX_HR(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

#ifdef ENABLE_GFX_DEBUG_NAME
        GFX_HR(m_CommandQueue->SetName(StringUtils::Utf8ToUtf16(name).c_str()));
#else
        (name);
#endif
    }

    GfxCommandListType GfxCommandQueue::GetType() const
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = m_CommandQueue->GetDesc();
        return GfxCommandList::FromD3D12Type(queueDesc.Type);
    }

    void GfxCommandQueue::ExecuteCommandList(GfxCommandList* commandList)
    {
        ID3D12CommandList* lists = commandList->GetD3D12CommandList();
        m_CommandQueue->ExecuteCommandLists(1, &lists);
    }

    void GfxCommandQueue::Wait(GfxFence* fence)
    {
        Wait(fence, fence->m_Value);
    }

    void GfxCommandQueue::Wait(GfxFence* fence, uint64_t value)
    {
        GFX_HR(m_CommandQueue->Wait(fence->GetD3D12Fence(), static_cast<UINT64>(value)));
    }

    uint64_t GfxCommandQueue::SignalNextValue(GfxFence* fence)
    {
        uint64_t value = ++fence->m_Value;
        GFX_HR(m_CommandQueue->Signal(fence->GetD3D12Fence(), static_cast<UINT64>(value)));
        return value;
    }
}
