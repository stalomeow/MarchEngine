#include "Rendering/Command/CommandBuffer.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"
#include <assert.h>
#include <utility>

namespace dx12demo
{
    std::unique_ptr<CommandAllocatorPool> CommandBuffer::s_CommandAllocatorPool = std::make_unique<CommandAllocatorPool>();
    std::vector<std::unique_ptr<CommandBuffer>> CommandBuffer::s_AllCommandBuffers{};
    std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::queue<CommandBuffer*>> CommandBuffer::s_FreeCommandBuffers{};

    CommandBuffer::CommandBuffer(D3D12_COMMAND_LIST_TYPE type)
        : m_Type(type)
    {
        m_CmdAllocator = s_CommandAllocatorPool->Get(m_Type);
        m_UploadHeapAllocator = std::make_unique<UploadHeapAllocator>(4096);

        auto device = GetGfxManager().GetDevice();
        THROW_IF_FAILED(device->CreateCommandList(0, type, m_CmdAllocator, nullptr, IID_PPV_ARGS(&m_CmdList)));
    }

    void CommandBuffer::Reset()
    {
        assert(m_CmdAllocator == nullptr);

        m_CmdAllocator = s_CommandAllocatorPool->Get(m_Type);
        THROW_IF_FAILED(m_CmdList->Reset(m_CmdAllocator, nullptr));
    }

    void CommandBuffer::ExecuteAndRelease(bool waitForCompletion)
    {
        THROW_IF_FAILED(m_CmdList->Close());
        ID3D12CommandList* cmdList = m_CmdList.Get();
        GetGfxManager().GetCommandQueue()->ExecuteCommandLists(1, &cmdList);

        UINT64 fenceValue = GetGfxManager().SignalNextFenceValue();
        s_CommandAllocatorPool->Release(std::exchange(m_CmdAllocator, nullptr), m_Type, fenceValue);
        m_UploadHeapAllocator->FlushPages(fenceValue);
        s_FreeCommandBuffers[m_Type].push(this);

        if (waitForCompletion)
        {
            GetGfxManager().WaitForFence(fenceValue);
        }
    }

    CommandBuffer* CommandBuffer::Get(D3D12_COMMAND_LIST_TYPE type)
    {
        std::queue<CommandBuffer*>& pool = s_FreeCommandBuffers[type];

        if (!pool.empty())
        {
            CommandBuffer* cmd = pool.front();
            pool.pop();
            cmd->Reset();
            return cmd;
        }

        s_AllCommandBuffers.push_back(std::make_unique<CommandBuffer>(type));
        return s_AllCommandBuffers.back().get();
    }
}
