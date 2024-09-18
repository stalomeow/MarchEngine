#include "GfxCommandList.h"
#include "GfxDevice.h"
#include "GfxExcept.h"
#include "StringUtility.h"
#include <assert.h>

namespace march
{
    GfxCommandList::GfxCommandList(GfxDevice* device, GfxCommandListType type, const std::string& name)
        : m_Device(device), m_Type(type), m_Name(name), m_CommandList(nullptr)
    {

    }

    void GfxCommandList::Begin(ID3D12CommandAllocator* commandAllocator)
    {
        if (m_CommandList == nullptr)
        {
            ID3D12Device4* d3d12Device = m_Device->GetD3D12Device();
            D3D12_COMMAND_LIST_TYPE type = ToD3D12Type(m_Type);
            GFX_HR(d3d12Device->CreateCommandList(0, type, commandAllocator, nullptr, IID_PPV_ARGS(m_CommandList.GetAddressOf())));

#ifdef ENABLE_GFX_DEBUG_NAME
            GFX_HR(m_CommandList->SetName(StringUtility::Utf8ToUtf16(m_Name).c_str()));
#endif
        }
        else
        {
            GFX_HR(m_CommandList->Reset(commandAllocator, nullptr));
        }
    }

    void GfxCommandList::End()
    {
        GFX_HR(m_CommandList->Close());
    }

    GfxCommandListType GfxCommandList::FromD3D12Type(D3D12_COMMAND_LIST_TYPE type)
    {
        switch (type)
        {
        case D3D12_COMMAND_LIST_TYPE_DIRECT:
            return GfxCommandListType::Graphics;

        case D3D12_COMMAND_LIST_TYPE_COMPUTE:
            return GfxCommandListType::Compute;

        case D3D12_COMMAND_LIST_TYPE_COPY:
            return GfxCommandListType::Copy;

        default:
            throw GfxException("Invalid command list type");
        }
    }

    D3D12_COMMAND_LIST_TYPE GfxCommandList::ToD3D12Type(GfxCommandListType type)
    {
        switch (type)
        {
        case march::GfxCommandListType::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;

        case march::GfxCommandListType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;

        case march::GfxCommandListType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;

        default:
            throw GfxException("Invalid command list type");
        }
    }
}
