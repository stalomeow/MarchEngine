#include "Rendering/Command/CommandAllocatorPool.h"
#include "Rendering/GfxManager.h"
#include "Rendering/DxException.h"
#include "Rendering/GfxManager.h"

using namespace Microsoft::WRL;

namespace dx12demo
{
    ID3D12CommandAllocator* CommandAllocatorPool::Get(D3D12_COMMAND_LIST_TYPE type)
    {
        std::queue<std::pair<UINT64, ID3D12CommandAllocator*>>& pool = m_Pools[type];

        if (!pool.empty() && pool.front().first <= GetGfxManager().GetCompletedFenceValue())
        {
            ID3D12CommandAllocator* allocator = pool.front().second;
            pool.pop();

            // Reuse the memory associated with command recording.
            // We can only reset when the associated command lists have finished execution on the GPU.
            THROW_IF_FAILED(allocator->Reset());
            return allocator;
        }

        auto device = GetGfxManager().GetDevice();
        ComPtr<ID3D12CommandAllocator> result = nullptr;
        THROW_IF_FAILED(device->CreateCommandAllocator(type, IID_PPV_ARGS(result.GetAddressOf())));
        m_Refs.push_back(result);
        return result.Get();
    }

    void CommandAllocatorPool::Release(ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type, UINT64 fenceValue)
    {
        m_Pools[type].emplace(fenceValue, allocator);
    }
}
