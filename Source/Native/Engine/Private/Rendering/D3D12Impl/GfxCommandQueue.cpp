#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include <limits>
#include <stdexcept>
#include <assert.h>

using Microsoft::WRL::ComPtr;

namespace march
{
    GfxCommandQueue::GfxCommandQueue(GfxDevice* device, const std::string& name, const GfxCommandQueueDesc& desc)
        : m_Device(device)
        , m_Type(desc.Type)
        , m_Queue(nullptr)
        , m_CommandAllocatorMutex{}
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

        {
            std::lock_guard<std::mutex> lock(m_CommandAllocatorMutex);

            if (!m_CommandAllocators.empty() && m_Fence->IsCompleted(m_CommandAllocators.front().first))
            {
                result = m_CommandAllocators.front().second;
                m_CommandAllocators.pop();
            }
        }

        if (result != nullptr)
        {
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

        {
            std::lock_guard<std::mutex> lock(m_CommandAllocatorMutex);

            m_CommandAllocators.emplace(syncPoint.m_Value, allocator);
        }

        return syncPoint;
    }

    GfxCommandManager::GfxCommandManager(GfxDevice* device)
        : m_Device(device)
        , m_ContextStore{}
        , m_CmdListStore{}
        , m_FreeContexts{}
        , m_CompletedFrameFence(0)
        , m_RHIMutex{}
        , m_MainThreadCVar{}
        , m_RHIThreadCVar{}
        , m_IsSwappingCmdListBuffers(false)
        , m_CmdBuffers{}
        , m_MainThreadCmdBufferIndex(0)
        , m_RHIThreadCmdBufferIndex(1)
        , m_CmdBufferVersion(0)
        , m_IsRHIThreadRunning(true)
        , m_IsRHIThreadExecuted(false)
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
            m_QueueData[i].FrameFence = std::make_unique<GfxFence>(device, queueName + "FrameFence", m_CompletedFrameFence.load(std::memory_order_relaxed));
            m_QueueData[i].FreeCmdLists = {};
        }

        m_RHIThread = std::thread(&GfxCommandManager::RHIThreadProc, this);
    }

    GfxCommandManager::~GfxCommandManager()
    {
        {
            std::unique_lock<std::mutex> lock(m_RHIMutex);
            m_IsRHIThreadRunning = false;
            m_RHIThreadCVar.notify_one();
        }

        if (m_RHIThread.joinable())
        {
            m_RHIThread.join();
        }
    }

    GfxCommandQueue* GfxCommandManager::GetQueue(GfxCommandType type) const
    {
        return m_QueueData[static_cast<size_t>(type)].Queue.get();
    }

    GfxCommandContext* GfxCommandManager::RequestContext(GfxCommandType type)
    {
        GfxCommandContext* context;

        if (!m_FreeContexts.empty())
        {
            context = m_FreeContexts.front();
            m_FreeContexts.pop();
        }
        else
        {
            m_ContextStore.emplace_back(std::make_unique<GfxCommandContext>(m_Device));
            context = m_ContextStore.back().get();
        }

        auto& queueData = m_QueueData[static_cast<size_t>(type)];
        GfxCommandList* commandList;

        if (!queueData.FreeCmdLists.empty())
        {
            commandList = queueData.FreeCmdLists.front();
            queueData.FreeCmdLists.pop();
        }
        else
        {
            m_CmdListStore.emplace_back(std::make_unique<GfxCommandList>(type, queueData.Queue.get()));
            commandList = m_CmdListStore.back().get();
        }

        context->Open(commandList);
        return context;
    }

    void GfxCommandManager::RecycleContext(GfxCommandContext* context)
    {
        m_FreeContexts.push(context);
    }

    uint64_t GfxCommandManager::GetCompletedFrameFence() const
    {
        return m_CompletedFrameFence.load(std::memory_order_relaxed);
    }

    bool GfxCommandManager::IsFrameFenceCompleted(uint64_t fence) const
    {
        return fence <= GetCompletedFrameFence();
    }

    uint64_t GfxCommandManager::GetNextFrameFence() const
    {
        // All queues should have the same value
        return m_QueueData[0].FrameFence->GetNextValue() + 1;
    }

    void GfxCommandManager::SignalNextFrameFence()
    {
        for (auto& data : m_QueueData)
        {
            data.FrameFence->SignalNextValueOnGpu(data.Queue->GetQueue());
        }
    }

    void GfxCommandManager::RefreshCompletedFrameFence(bool waitForLastFrame)
    {
        if (waitForLastFrame)
        {
            // All queues should have the same value
            uint64_t fence = m_QueueData[0].FrameFence->GetNextValue();

            for (auto& data : m_QueueData)
            {
                data.FrameFence->WaitOnCpu(fence);
            }
        }

        // Refresh the completed frame fence
        uint64_t fence = std::numeric_limits<uint64_t>::max();

        for (auto& data : m_QueueData)
        {
            fence = std::min(fence, data.FrameFence->GetCompletedValue());
        }

        m_CompletedFrameFence.store(fence, std::memory_order_relaxed);
    }

    void GfxCommandManager::RecycleCommandList(GfxCommandList* list)
    {
        m_QueueData[static_cast<size_t>(list->GetType())].FreeCmdLists.push(list);
    }

    void GfxCommandManager::RHIThreadProc()
    {
        std::vector<GfxSyncPoint> resolvedSyncPoints{};

        auto resolveFunc = [&](const GfxFutureSyncPoint& syncPoint) -> GfxSyncPoint
        {
            // sync point 是上一帧的，所以必须是上一个 version
            assert(syncPoint.Version == m_CmdBufferVersion - 1);

            return resolvedSyncPoints[syncPoint.Index];
        };

        while (SyncOnRHIThread())
        {
            resolvedSyncPoints.clear();
            GetGfxDevice()->GetOnlineViewDescriptorAllocator()->CleanUpAllocations();
            GetGfxDevice()->GetOnlineSamplerDescriptorAllocator()->CleanUpAllocations();

            for (const CommandType& c : m_CmdBuffers[m_RHIThreadCmdBufferIndex])
            {
                std::visit([&](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;

                    GfxSyncPoint syncPoint{};

                    if constexpr (std::is_same_v<T, GfxCommandList*>)
                    {
                        arg->ResolveFutureSyncPoints(resolveFunc);
                        syncPoint = arg->Execute(/* isImmediateMode */ false);
                    }
                    else if constexpr (std::is_same_v<T, GfxSyncPoint>)
                    {
                        GetQueue(GfxCommandType::Direct)->WaitOnGpu(arg);
                    }
                    else if constexpr (std::is_same_v<T, GfxFutureSyncPoint>)
                    {
                        GetQueue(GfxCommandType::Direct)->WaitOnGpu(resolveFunc(arg));
                    }

                    resolvedSyncPoints.emplace_back(syncPoint);
                }, c);
            }

            m_IsRHIThreadExecuted.store(true, std::memory_order_release);
        }
    }

    GfxFutureSyncPoint GfxCommandManager::Execute(GfxCommandList* list)
    {
        std::vector<CommandType>& listBuffer = m_CmdBuffers[m_MainThreadCmdBufferIndex];
        size_t index = listBuffer.size();
        listBuffer.emplace_back(list);
        return GfxFutureSyncPoint{ index, m_CmdBufferVersion };
    }

    GfxSyncPoint GfxCommandManager::ExecuteImmediate(GfxCommandList* list)
    {
        if (list->HasFutureSyncPoints())
        {
            throw GfxException("Cannot immediately execute a command list with future sync points.");
        }

        GfxSyncPoint syncPoint = list->Execute(/* isImmediateMode */ true);
        RecycleCommandList(list);
        return syncPoint;
    }

    void GfxCommandManager::WaitOnGpu(const GfxSyncPoint& syncPoint)
    {
        m_CmdBuffers[m_MainThreadCmdBufferIndex].emplace_back(syncPoint);
    }

    void GfxCommandManager::WaitOnGpu(const GfxFutureSyncPoint& syncPoint)
    {
        m_CmdBuffers[m_MainThreadCmdBufferIndex].emplace_back(syncPoint);
    }

    bool GfxCommandManager::SyncOnRHIThread()
    {
        std::unique_lock<std::mutex> lock(m_RHIMutex);

        // 等待 Main Thread 发送交换 Buffer 的通知
        m_RHIThreadCVar.wait(lock, [this]() { return m_IsSwappingCmdListBuffers || !m_IsRHIThreadRunning; });

        if (m_IsRHIThreadRunning && m_IsSwappingCmdListBuffers)
        {
            // 回收在 RHI Thread 上执行完的 Command List
            std::vector<CommandType>& rhiCmds = m_CmdBuffers[m_RHIThreadCmdBufferIndex];
            for (const auto& c : rhiCmds)
            {
                std::visit([this](auto&& arg)
                {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, GfxCommandList*>)
                    {
                        RecycleCommandList(arg);
                    }
                }, c);
            }
            rhiCmds.clear();

            // 交换 Buffer
            std::swap(m_MainThreadCmdBufferIndex, m_RHIThreadCmdBufferIndex);
            m_CmdBufferVersion++;

            // 通知 Main Thread 交换完成
            m_IsSwappingCmdListBuffers = false;
            m_MainThreadCVar.notify_one();
        }

        return m_IsRHIThreadRunning;
    }

    bool GfxCommandManager::SyncOnMainThread()
    {
        std::unique_lock<std::mutex> lock(m_RHIMutex);

        // 通知 RHI 线程交换 Buffer
        m_IsSwappingCmdListBuffers = true;
        m_RHIThreadCVar.notify_one();

        // 等待 RHI 线程交换完成
        m_MainThreadCVar.wait(lock, [this]() { return !m_IsSwappingCmdListBuffers; });

        // 返回 RHI 线程的执行结果
        return m_IsRHIThreadExecuted.load(std::memory_order_acquire);
    }
}
