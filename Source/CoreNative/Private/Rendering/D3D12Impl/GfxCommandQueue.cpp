#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include <limits>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace march
{
    GfxCommandQueue::GfxCommandQueue(GfxDevice* device, const std::string& name, const GfxCommandQueueDesc& desc)
        : m_Device(device)
        , m_Type(desc.Type)
        , m_Queue(nullptr)
        , m_CommandAllocators{}
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = desc.Type;
        queueDesc.Priority = static_cast<INT>(desc.Priority);

        if (desc.DisableGpuTimeout)
        {
            queueDesc.Flags |= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
        }

        CHECK_HR(device->GetD3DDevice4()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue)));
        GfxUtils::SetName(m_Queue.Get(), name);

        m_Fence = std::make_unique<GfxFence>(device, name + "PrivateFence");
    }

    GfxSyncPoint GfxCommandQueue::CreateSyncPoint()
    {
        uint64_t value = m_Fence->SignalNextValueOnGpu(m_Queue.Get());
        return GfxSyncPoint{ m_Fence.get(), value };
    }

    void GfxCommandQueue::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        syncPoint.m_Fence->WaitOnGpu(m_Queue.Get(), syncPoint.m_Value);
    }

    ComPtr<ID3D12CommandAllocator> GfxCommandQueue::RequestCommandAllocator()
    {
        ComPtr<ID3D12CommandAllocator> result = nullptr;

        if (!m_CommandAllocators.empty() && m_Fence->IsCompleted(m_CommandAllocators.front().first))
        {
            result = m_CommandAllocators.front().second;
            m_CommandAllocators.pop();

            // Reuse the memory associated with command recording.
            // We can only reset when the associated command lists have finished execution on the GPU.
            CHECK_HR(result->Reset());
        }
        else
        {
            CHECK_HR(m_Device->GetD3DDevice4()->CreateCommandAllocator(m_Type, IID_PPV_ARGS(&result)));
        }

        return result;
    }

    GfxSyncPoint GfxCommandQueue::ReleaseCommandAllocator(ComPtr<ID3D12CommandAllocator> allocator)
    {
        GfxSyncPoint syncPoint = CreateSyncPoint();
        m_CommandAllocators.emplace(syncPoint.m_Value, allocator);
        return syncPoint;
    }

    GfxCommandManager::GfxCommandManager(GfxDevice* device)
        : m_Device(device)
        , m_ContextStore{}
        , m_CompletedFrameFence(0)
    {
        GfxCommandQueueDesc queueDesc{};
        queueDesc.Priority = 0;
        queueDesc.DisableGpuTimeout = false;

        for (size_t i = 0; i < std::size(m_QueueData); i++)
        {
            std::string queueName;

            switch (static_cast<GfxCommandType>(i))
            {
            case GfxCommandType::Direct:
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                queueName = "DirectQueue";
                break;
            case GfxCommandType::AsyncCompute:
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
                queueName = "AsyncComputeQueue";
                break;
            case GfxCommandType::AsyncCopy:
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
                queueName = "AsyncCopyQueue";
                break;
            default:
                throw std::runtime_error("Unsupported command type");
            }

            m_QueueData[i].Queue = std::make_unique<GfxCommandQueue>(device, queueName, queueDesc);
            m_QueueData[i].FrameFence = std::make_unique<GfxFence>(device, queueName + "FrameFence", m_CompletedFrameFence);
            m_QueueData[i].FreeContexts = {};
        }
    }

    GfxCommandQueue* GfxCommandManager::GetQueue(GfxCommandType type) const
    {
        return m_QueueData[static_cast<size_t>(type)].Queue.get();
    }

    GfxCommandContext* GfxCommandManager::RequestAndOpenContext(GfxCommandType type)
    {
        std::queue<GfxCommandContext*>& q = m_QueueData[static_cast<size_t>(type)].FreeContexts;
        GfxCommandContext* result;

        if (!q.empty())
        {
            result = q.front();
            q.pop();
        }
        else
        {
            m_ContextStore.push_back(std::make_unique<GfxCommandContext>(m_Device, type));
            result = m_ContextStore.back().get();
        }

        result->Open();
        return result;
    }

    void GfxCommandManager::RecycleContext(GfxCommandContext* context)
    {
        m_QueueData[static_cast<size_t>(context->GetType())].FreeContexts.push(context);
    }

    uint64_t GfxCommandManager::GetCompletedFrameFence() const
    {
        return m_CompletedFrameFence;
    }

    bool GfxCommandManager::IsFrameFenceCompleted(uint64_t fence) const
    {
        return fence <= GetCompletedFrameFence();
    }

    uint64_t GfxCommandManager::GetNextFrameFence() const
    {
        // All queues should have the same value
        return m_QueueData[0].FrameFence->GetNextValue();
    }

    void GfxCommandManager::SignalNextFrameFence(bool waitForGpuIdle)
    {
        uint64_t fence;

        for (auto& data : m_QueueData)
        {
            // All queues should have the same value
            fence = data.FrameFence->SignalNextValueOnGpu(data.Queue->GetQueue());
        }

        if (waitForGpuIdle)
        {
            for (auto& data : m_QueueData)
            {
                data.FrameFence->WaitOnCpu(fence);
            }
        }

        // Refresh the completed frame fence
        m_CompletedFrameFence = std::numeric_limits<uint64_t>::max();

        for (auto& data : m_QueueData)
        {
            m_CompletedFrameFence = std::min(m_CompletedFrameFence, data.FrameFence->GetCompletedValue());
        }
    }
}
