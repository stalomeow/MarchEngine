#include "Rendering/Command/CommandAllocatorPool.h"
#include "Rendering/DxException.h"

using namespace Microsoft::WRL;

namespace dx12demo
{
    CommandAllocatorPool::CommandAllocatorPool(ComPtr<ID3D12Device> device, D3D12_COMMAND_LIST_TYPE type)
        : m_Device(device), m_CmdListType(type), m_AllocatorRefs{}, m_AllocatorPool{}
    {

    }

    ID3D12CommandAllocator* CommandAllocatorPool::Get(UINT64 completedFenceValue)
    {
        if (!m_AllocatorPool.empty() && m_AllocatorPool.front().first <= completedFenceValue)
        {
            ID3D12CommandAllocator* allocator = m_AllocatorPool.front().second;
            m_AllocatorPool.pop();

            // Reuse the memory associated with command recording.
            // We can only reset when the associated command lists have finished execution on the GPU.
            THROW_IF_FAILED(allocator->Reset());
            return allocator;
        }

        ComPtr<ID3D12CommandAllocator> result = nullptr;
        THROW_IF_FAILED(m_Device->CreateCommandAllocator(m_CmdListType, IID_PPV_ARGS(result.GetAddressOf())));
        m_AllocatorRefs.push_back(result);
        return result.Get();
    }

    void CommandAllocatorPool::Release(ID3D12CommandAllocator* allocator, UINT64 fenceValue)
    {
        m_AllocatorPool.push(std::make_pair(fenceValue, allocator));
    }
}
