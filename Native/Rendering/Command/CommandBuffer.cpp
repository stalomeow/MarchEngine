#include "Rendering/Command/CommandBuffer.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"
#include "Core/Debug.h"
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
        m_UploadHeapAllocator = std::make_unique<UploadHeapAllocator>(4 * 1024 * 1024 /* 4 MB */);

        auto device = GetGfxManager().GetDevice();
        THROW_IF_FAILED(device->CreateCommandList(0, type, m_CmdAllocator, nullptr, IID_PPV_ARGS(&m_CmdList)));

        SetDescriptorHeaps();
    }

    void CommandBuffer::Reset()
    {
        assert(m_CmdAllocator == nullptr);

        m_CmdAllocator = s_CommandAllocatorPool->Get(m_Type);
        THROW_IF_FAILED(m_CmdList->Reset(m_CmdAllocator, nullptr));

        SetDescriptorHeaps();
    }

    void CommandBuffer::SetDescriptorHeaps()
    {
        DescriptorTableAllocator* allocator1 = GetGfxManager().GetViewDescriptorTableAllocator();
        DescriptorTableAllocator* allocator2 = GetGfxManager().GetSamplerDescriptorTableAllocator();
        ID3D12DescriptorHeap* heaps[] = { allocator1->GetHeapPointer(), allocator2->GetHeapPointer() };
        m_CmdList->SetDescriptorHeaps(2, heaps);
    }

    DescriptorTable CommandBuffer::AllocateTempViewDescriptorTable(UINT descriptorCount)
    {
        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();
        DescriptorTableAllocator* allocator = GetGfxManager().GetViewDescriptorTableAllocator();
        DescriptorTable table = allocator->AllocateDynamicTable(descriptorCount, completedFenceValue);
        m_TempDescriptorTables.push_back(table);
        return table;
    }

    DescriptorTable CommandBuffer::AllocateTempSamplerDescriptorTable(UINT descriptorCount)
    {
        UINT64 completedFenceValue = GetGfxManager().GetCompletedFenceValue();
        DescriptorTableAllocator* allocator = GetGfxManager().GetSamplerDescriptorTableAllocator();
        DescriptorTable table = allocator->AllocateDynamicTable(descriptorCount, completedFenceValue);
        m_TempDescriptorTables.push_back(table);
        return table;
    }

    void CommandBuffer::ExecuteAndRelease(bool waitForCompletion)
    {
        THROW_IF_FAILED(m_CmdList->Close());
        ID3D12CommandList* cmdList = m_CmdList.Get();
        GetGfxManager().GetCommandQueue(m_Type)->ExecuteCommandLists(1, &cmdList);

        UINT64 fenceValue = GetGfxManager().SignalNextFenceValue();
        s_CommandAllocatorPool->Release(std::exchange(m_CmdAllocator, nullptr), m_Type, fenceValue);
        m_UploadHeapAllocator->FlushPages(fenceValue);

        for (const DescriptorTable& table : m_TempDescriptorTables)
        {
            switch (table.GetType())
            {
            case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
                GetGfxManager().GetViewDescriptorTableAllocator()->ReleaseDynamicTables(1, &table, fenceValue);
                break;
            case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
                GetGfxManager().GetSamplerDescriptorTableAllocator()->ReleaseDynamicTables(1, &table, fenceValue);
                break;
            default:
                DEBUG_LOG_ERROR("Unknown descriptor table type");
                break;
            }
        }

        m_TempDescriptorTables.clear();
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
