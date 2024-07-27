#include "Rendering/Command/CommandContext.h"
#include "Rendering/DxException.h"
#include <assert.h>
#include <utility>

namespace dx12demo
{
    CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE type)
        : m_Type(type)
    {

    }

    void CommandContext::Initialize(ID3D12Device* device, ID3D12CommandAllocator* allocator)
    {
        m_Allocator = allocator;
        THROW_IF_FAILED(device->CreateCommandList(0, m_Type, allocator, nullptr,
            IID_PPV_ARGS(m_List.GetAddressOf())));
    }

    void CommandContext::Reset(ID3D12CommandAllocator* allocator)
    {
        assert(m_Allocator == nullptr);
        assert(m_List != nullptr);

        m_Allocator = allocator;
        THROW_IF_FAILED(m_List->Reset(allocator, nullptr));
    }

    ID3D12CommandAllocator* CommandContext::Close()
    {
        THROW_IF_FAILED(m_List->Close());
        return std::exchange(m_Allocator, nullptr);
    }
}
