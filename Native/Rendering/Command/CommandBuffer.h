#pragma once

#include "Rendering/Resource/UploadHeapAllocator.h"
#include "Rendering/Command/CommandAllocatorPool.h"
#include "Rendering/DescriptorHeap.h"
#include <d3d12.h>
#include <wrl.h>
#include <memory>
#include <queue>
#include <vector>
#include <unordered_map>

namespace dx12demo
{
    class CommandBuffer
    {
    public:
        CommandBuffer(D3D12_COMMAND_LIST_TYPE type);
        ~CommandBuffer() = default;

        template<typename T = BYTE>
        UploadHeapSpan<T> AllocateTempUploadHeap(UINT count, UINT alignment = 1) { return m_UploadHeapAllocator->Allocate<T>(count, alignment); }
        DescriptorHeapSpan AllocateTempCbvSrvUavHeap(UINT count) { return m_CbvSrvUavHeapAllocator->Allocate(count); }
        DescriptorHeapSpan AllocateTempSamplerHeap(UINT count) { return m_SamplerHeapAllocator->Allocate(count); }
        void ExecuteAndRelease(bool waitForCompletion = false);

        D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
        ID3D12GraphicsCommandList* GetList() const { return m_CmdList.Get(); }

    protected:
        void Reset();

    private:
        D3D12_COMMAND_LIST_TYPE m_Type;
        ID3D12CommandAllocator* m_CmdAllocator; // We don't own this
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CmdList;
        std::unique_ptr<UploadHeapAllocator> m_UploadHeapAllocator;
        std::unique_ptr<DescriptorHeapAllocator> m_CbvSrvUavHeapAllocator;
        std::unique_ptr<DescriptorHeapAllocator> m_SamplerHeapAllocator;

    public:
        static CommandBuffer* Get(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);

    private:
        static std::unique_ptr<CommandAllocatorPool> s_CommandAllocatorPool;
        static std::vector<std::unique_ptr<CommandBuffer>> s_AllCommandBuffers;
        static std::unordered_map<D3D12_COMMAND_LIST_TYPE, std::queue<CommandBuffer*>> s_FreeCommandBuffers;
    };
}
