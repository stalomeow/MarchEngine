#pragma once

#include <directx/d3dx12.h>
#include <wrl.h>
#include <string>

namespace march
{
    class GfxDevice;

    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization
    enum class GfxCommandListType
    {
        Graphics,
        Compute,
        Copy,
        NumCommandList,
    };

    class GfxCommandList
    {
    public:
        GfxCommandList(GfxDevice* device, GfxCommandListType type, const std::string& name);
        ~GfxCommandList() = default;

        void Begin(ID3D12CommandAllocator* commandAllocator);
        void End();

        GfxCommandListType GetType() const { return m_Type; }
        ID3D12GraphicsCommandList* GetD3D12CommandList() const { return m_CommandList.Get(); }

    private:
        GfxDevice* m_Device;
        GfxCommandListType m_Type;
        std::string m_Name;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;

    public:
        static GfxCommandListType FromD3D12Type(D3D12_COMMAND_LIST_TYPE type);
        static D3D12_COMMAND_LIST_TYPE ToD3D12Type(GfxCommandListType type);
    };
}
