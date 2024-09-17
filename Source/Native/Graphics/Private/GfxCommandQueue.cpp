#include "GfxCommandQueue.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "GfxFence.h"
#include "GfxCommandList.h"
#include "StringUtility.h"
#include <Windows.h>

namespace march
{
    GfxCommandQueue::GfxCommandQueue(GfxDevice* device, GfxCommandListType type, const std::string& name, int32_t priority, bool disableGpuTimeout)
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};

        switch (type)
        {
        case march::GfxCommandListType::Graphics:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            break;
        case march::GfxCommandListType::Compute:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            break;
        case march::GfxCommandListType::Copy:
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            break;
        default:
            throw GfxException("Invalid command queue type");
        }

        queueDesc.Priority = priority;

        if (disableGpuTimeout)
        {
            queueDesc.Flags |= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        }

        ID3D12Device4* d3d12Device = device->GetD3D12Device();
        GFX_HR(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

#ifdef ENABLE_GFX_DEBUG_NAME
        GFX_HR(m_CommandQueue->SetName(StringUtility::Utf8ToUtf16(name).c_str()));
#else
        (name);
#endif
    }

    GfxCommandListType GfxCommandQueue::GetType() const
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = m_CommandQueue->GetDesc();

        switch (queueDesc.Type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return GfxCommandListType::Graphics;

        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return GfxCommandListType::Compute;

        case D3D12_COMMAND_LIST_TYPE_COPY:
            return GfxCommandListType::Copy;

        default:
            throw GfxException("Invalid command queue type");
        }
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
