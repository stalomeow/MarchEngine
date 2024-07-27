#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <vector>

namespace dx12demo
{
    class CommandAllocatorPool
    {
    public:
        CommandAllocatorPool(Microsoft::WRL::ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type);
        ~CommandAllocatorPool() = default;

        ID3D12CommandAllocator* Get(UINT64 completedFenceValue);
        void Release(ID3D12CommandAllocator* allocator, UINT64 fenceValue);

    private:
        Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
        D3D12_COMMAND_LIST_TYPE m_CmdListType;

        std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_AllocatorRefs;
        std::queue<std::pair<UINT64, ID3D12CommandAllocator*>> m_AllocatorPool;
    };
}
