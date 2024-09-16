#pragma once

#include "UploadHeapAllocator.h"
#include "CommandAllocatorPool.h"
#include "DescriptorHeap.h"
#include <directx/d3dx12.h>
#include <wrl.h>
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>

namespace march
{
    class CommandBuffer
    {
    public:
        CommandBuffer(D3D12_COMMAND_LIST_TYPE type);
        ~CommandBuffer() = default;

        template<typename T = BYTE>
        UploadHeapSpan<T> AllocateTempUploadHeap(UINT count, UINT alignment = 1) { return m_UploadHeapAllocator->Allocate<T>(count, alignment); }
        DescriptorTable AllocateTempViewDescriptorTable(UINT descriptorCount);
        DescriptorTable AllocateTempSamplerDescriptorTable(UINT descriptorCount);
        void ExecuteAndRelease(bool waitForCompletion = false);

        D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
        ID3D12GraphicsCommandList* GetList() const { return m_CmdList.Get(); }

    protected:
        void Reset();
        void SetDescriptorHeaps();

    private:
        D3D12_COMMAND_LIST_TYPE m_Type;
        ID3D12CommandAllocator* m_CmdAllocator; // We don't own this
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CmdList;
        std::unique_ptr<UploadHeapAllocator> m_UploadHeapAllocator;
        std::vector<DescriptorTable> m_TempDescriptorTables{};

    public:
        static CommandBuffer* Get(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    private:
        static std::unique_ptr<CommandAllocatorPool> s_CommandAllocatorPool;
        static std::vector<std::unique_ptr<CommandBuffer>> s_AllCommandBuffers;
        static std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::queue<CommandBuffer*>> s_FreeCommandBuffers;
    };
}
