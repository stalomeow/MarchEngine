#include "GfxCommand.h"
#include "GfxDevice.h"
#include "StringUtils.h"
#include <Windows.h>

namespace march
{
    GfxFence::GfxFence(GfxDevice* device, const std::string& name, uint64_t initialValue)
        : GfxDeviceChild(device)
        , m_Value(initialValue)
    {
        D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE;
        ID3D12Device4* d3d12Device = device->GetD3D12Device();
        GFX_HR(d3d12Device->CreateFence(static_cast<UINT64>(initialValue), flags, IID_PPV_ARGS(&m_Fence)));

#ifdef ENABLE_GFX_DEBUG_NAME
        GFX_HR(m_Fence->SetName(StringUtils::Utf8ToUtf16(name).c_str()));
#else
        (name);
#endif

        m_EventHandle = CreateEventExW(NULL, NULL, NULL, EVENT_ALL_ACCESS);
    }

    GfxFence::~GfxFence()
    {
        CloseHandle(m_EventHandle);
    }

    uint64_t GfxFence::GetCompletedValue() const
    {
        return static_cast<uint64_t>(m_Fence->GetCompletedValue());
    }

    bool GfxFence::IsCompleted(uint64_t value) const
    {
        return static_cast<UINT64>(value) <= m_Fence->GetCompletedValue();
    }

    void GfxFence::Wait() const
    {
        Wait(m_Value);
    }

    void GfxFence::Wait(uint64_t value) const
    {
        if (m_Fence->GetCompletedValue() < static_cast<UINT64>(value))
        {
            GFX_HR(m_Fence->SetEventOnCompletion(static_cast<UINT64>(value), m_EventHandle));
            WaitForSingleObject(m_EventHandle, INFINITE);
        }
    }

    uint64_t GfxFence::SignalNextValue()
    {
        uint64_t value = ++m_Value;
        GFX_HR(m_Fence->Signal(static_cast<UINT64>(value)));
        return value;
    }

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
