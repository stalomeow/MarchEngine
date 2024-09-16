#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Texture.h"
#include "DxException.h"
#include "CommandBuffer.h"
#include <vector>

using namespace DirectX;

namespace march
{
    Texture::Texture()
    {
        m_TextureDescriptorHandle = GetGfxManager().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SamplerDescriptorHandle = GetGfxManager().AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        UpdateSampler();
    }

    Texture::~Texture()
    {
        GetGfxManager().FreeDescriptor(m_TextureDescriptorHandle);
        GetGfxManager().FreeDescriptor(m_SamplerDescriptorHandle);
    }

    void Texture::SetDDSData(const std::wstring& name, const void* pSourceDDS, size_t size)
    {
        if (m_Resource != nullptr)
        {
            GetGfxManager().SafeReleaseObject(m_Resource);
            m_Resource = nullptr;
        }

        // https://github.com/microsoft/DirectXTex/wiki/CreateTexture#directx-12

        ScratchImage image;
        THROW_IF_FAILED(LoadFromDDSMemory(pSourceDDS, size, DDS_FLAGS_NONE, &m_MetaData, image));

        ID3D12Device* device = GetGfxManager().GetDevice();
        THROW_IF_FAILED(CreateTexture(device, m_MetaData, &m_Resource));
        m_State = D3D12_RESOURCE_STATE_COMMON; // CreateTexture 使用的 state

#if defined(ENABLE_GFX_DEBUG_NAME)
        m_Resource->SetName(name.c_str());
#endif

        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        THROW_IF_FAILED(PrepareUpload(device, image.GetImages(), image.GetImageCount(), m_MetaData, subresources));

        // upload is implemented by application developer. Here's one solution using <d3dx12.h>
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_Resource, 0, static_cast<UINT>(subresources.size()));

        CommandBuffer* cmd = CommandBuffer::Get();
        UploadHeapSpan<BYTE> span = cmd->AllocateTempUploadHeap(uploadBufferSize);
        UpdateSubresources(cmd->GetList(), m_Resource, span.GetResource(), span.GetOffsetInResource(),
            0, static_cast<UINT>(subresources.size()), subresources.data());
        cmd->ExecuteAndRelease(true); // 等待上传完成，ScratchImage 等会被析构

        device->CreateShaderResourceView(m_Resource, nullptr, m_TextureDescriptorHandle.GetCpuHandle());
    }

    void Texture::UpdateSampler() const
    {
        D3D12_SAMPLER_DESC samplerDesc = {};

        switch (m_FilterMode)
        {
        case FilterMode::Point:
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            break;
        case FilterMode::Bilinear:
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            break;
        case FilterMode::Trilinear:
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;
        default:
            throw std::invalid_argument("Invalid filter mode");
        }

        switch (m_WrapMode)
        {
        case march::WrapMode::Repeat:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        case march::WrapMode::Clamp:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        case march::WrapMode::Mirror:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
            break;
        default:
            throw std::invalid_argument("Invalid wrap mode");
        }

        samplerDesc.MipLODBias = 0;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        samplerDesc.MinLOD = 0;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

        ID3D12Device* device = GetGfxManager().GetDevice();
        device->CreateSampler(&samplerDesc, m_SamplerDescriptorHandle.GetCpuHandle());
    }

    Texture* Texture::s_pBlackTexture = nullptr;
    Texture* Texture::s_pWhiteTexture = nullptr;

    Texture* Texture::GetDefaultBlack()
    {
        static const uint8_t ddsData[] =
        {
            0x44, 0x44, 0x53, 0x20, 0x7C, 0x00, 0x00, 0x00, 0x0F, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x47, 0x49, 0x4D, 0x50, 0x2D, 0x44, 0x44, 0x53, 0x5C, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
            0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
            0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x10, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0xFF,
        };

        if (s_pBlackTexture == nullptr)
        {
            s_pBlackTexture = new Texture();
            s_pBlackTexture->SetDDSData(L"DefaultBlackTexture", ddsData, sizeof(ddsData));
            s_pBlackTexture->SetFilterAndWrapMode(FilterMode::Point, WrapMode::Clamp);
        }

        return s_pBlackTexture;
    }

    Texture* Texture::GetDefaultWhite()
    {
        static const uint8_t ddsData[] =
        {
            0x44, 0x44, 0x53, 0x20, 0x7C, 0x00, 0x00, 0x00, 0x0F, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x47, 0x49, 0x4D, 0x50, 0x2D, 0x44, 0x44, 0x53, 0x5C, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
            0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00,
            0x00, 0xFF, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x10, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xFF, 0xFF, 0xFF, 0xFF,
        };

        if (s_pWhiteTexture == nullptr)
        {
            s_pWhiteTexture = new Texture();
            s_pWhiteTexture->SetDDSData(L"DefaultWhiteTexture", ddsData, sizeof(ddsData));
            s_pWhiteTexture->SetFilterAndWrapMode(FilterMode::Point, WrapMode::Clamp);
        }

        return s_pWhiteTexture;
    }
}
