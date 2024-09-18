#include "GfxCommandAllocatorPool.h"
#include "GfxCommandList.h"
#include "GfxDevice.h"
#include "GfxFence.h"
#include "GfxExcept.h"

using namespace Microsoft::WRL;

namespace march
{
    GfxCommandAllocatorPool::GfxCommandAllocatorPool(GfxDevice* device, GfxCommandListType type)
        : m_Device(device), m_Type(type), m_Allocators{}, m_UsedAllocators{}, m_ReleaseQueue{}
    {

    }

    void GfxCommandAllocatorPool::BeginFrame()
    {

    }

    void GfxCommandAllocatorPool::EndFrame(uint64_t fenceValue)
    {
        for (ID3D12CommandAllocator* allocator : m_UsedAllocators)
        {
            m_ReleaseQueue.emplace(fenceValue, allocator);
        }

        m_UsedAllocators.clear();
    }

    ID3D12CommandAllocator* GfxCommandAllocatorPool::Get()
    {
        ID3D12CommandAllocator* result;

        if (!m_ReleaseQueue.empty() && m_Device->GetGraphicsFence()->IsCompleted(m_ReleaseQueue.front().first))
        {
            result = m_ReleaseQueue.front().second;
            m_ReleaseQueue.pop();

            // Reuse the memory associated with command recording.
            // We can only reset when the associated command lists have finished execution on the GPU.
            GFX_HR(result->Reset());
        }
        else
        {
            ComPtr<ID3D12CommandAllocator> allocator = nullptr;
            ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();
            D3D12_COMMAND_LIST_TYPE type = GfxCommandList::ToD3D12Type(m_Type);
            GFX_HR(d3d12Device->CreateCommandAllocator(type, IID_PPV_ARGS(allocator.GetAddressOf())));

            m_Allocators.push_back(allocator);
            result = allocator.Get();
        }

        m_UsedAllocators.push_back(result);
        return result;
    }
}
