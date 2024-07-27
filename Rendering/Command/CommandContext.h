#pragma once

#include <d3d12.h>
#include <wrl.h>

namespace dx12demo
{
    class CommandContext
    {
        friend class GfxManager;

    public:
        CommandContext(D3D12_COMMAND_LIST_TYPE type);
        ~CommandContext() = default;

        D3D12_COMMAND_LIST_TYPE GetType() const { return m_Type; }
        ID3D12GraphicsCommandList* GetList() const { return m_List.Get(); }

    protected:
        void Initialize(ID3D12Device* device, ID3D12CommandAllocator* allocator);
        void Reset(ID3D12CommandAllocator* allocator);
        ID3D12CommandAllocator* Close();

    private:
        D3D12_COMMAND_LIST_TYPE m_Type;
        ID3D12CommandAllocator* m_Allocator = nullptr; // We don't own this
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_List = nullptr;
    };
}
