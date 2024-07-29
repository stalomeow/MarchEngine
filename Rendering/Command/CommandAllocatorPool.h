#pragma once

#include <directx/d3dx12.h>
#include <d3d12.h>
#include <wrl.h>
#include <queue>
#include <vector>
#include <unordered_map>

namespace dx12demo
{
    class CommandAllocatorPool
    {
    public:
        CommandAllocatorPool() = default;
        ~CommandAllocatorPool() = default;

        ID3D12CommandAllocator* Get(D3D12_COMMAND_LIST_TYPE type);
        void Release(ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type, UINT64 fenceValue);

    private:
        std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_Refs{};
        std::unordered_map<D3D12_COMMAND_LIST_TYPE,
            std::queue<std::pair<UINT64, ID3D12CommandAllocator*>>> m_Pools{};
    };
}
