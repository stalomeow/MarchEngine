#include "GfxTexture.h"
#include "GfxDevice.h"
#include "GfxBuffer.h"
#include "GfxCommandList.h"
#include "GfxExcept.h"
#include "GfxSupportInfo.h"
#include <stdexcept>
#include <DirectXColors.h>

using namespace DirectX;

namespace march
{
    GfxTexture::GfxTexture(GfxDevice* device)
        : GfxResource(device, D3D12_RESOURCE_STATE_COMMON)
        , m_FilterMode(GfxFilterMode::Point), m_WrapMode(GfxWrapMode::Repeat)
    {
        m_SrvDescriptorHandle = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SamplerDescriptorHandle = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        UpdateSampler();
    }

    GfxTexture::~GfxTexture()
    {
        m_Device->FreeDescriptor(m_SrvDescriptorHandle);
        m_Device->FreeDescriptor(m_SamplerDescriptorHandle);
    }

    void GfxTexture::SetFilterMode(GfxFilterMode mode)
    {
        m_FilterMode = mode;
        UpdateSampler();
    }

    void GfxTexture::SetWrapMode(GfxWrapMode mode)
    {
        m_WrapMode = mode;
        UpdateSampler();
    }

    void GfxTexture::SetFilterAndWrapMode(GfxFilterMode filterMode, GfxWrapMode wrapMode)
    {
        m_FilterMode = filterMode;
        m_WrapMode = wrapMode;
        UpdateSampler();
    }

    void GfxTexture::UpdateSampler() const
    {
        D3D12_SAMPLER_DESC samplerDesc = {};

        switch (m_FilterMode)
        {
        case GfxFilterMode::Point:
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            break;
        case GfxFilterMode::Bilinear:
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            break;
        case GfxFilterMode::Trilinear:
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;
        default:
            throw std::invalid_argument("Invalid filter mode");
        }

        switch (m_WrapMode)
        {
        case GfxWrapMode::Repeat:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        case GfxWrapMode::Clamp:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        case GfxWrapMode::Mirror:
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

        ID3D12Device* device = m_Device->GetD3D12Device();
        device->CreateSampler(&samplerDesc, m_SamplerDescriptorHandle.GetCpuHandle());
    }

    std::unique_ptr<GfxTexture2D> GfxTexture::s_pBlackTexture = nullptr;
    std::unique_ptr<GfxTexture2D> GfxTexture::s_pWhiteTexture = nullptr;

    GfxTexture* GfxTexture::GetDefaultBlack()
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
            s_pBlackTexture = std::make_unique<GfxTexture2D>(GetGfxDevice());
            s_pBlackTexture->LoadFromDDS("DefaultBlackTexture", ddsData, sizeof(ddsData));
            s_pBlackTexture->SetFilterAndWrapMode(GfxFilterMode::Point, GfxWrapMode::Clamp);
        }

        return s_pBlackTexture.get();
    }

    GfxTexture* GfxTexture::GetDefaultWhite()
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
            s_pWhiteTexture = std::make_unique<GfxTexture2D>(GetGfxDevice());
            s_pWhiteTexture->LoadFromDDS("DefaultWhiteTexture", ddsData, sizeof(ddsData));
            s_pWhiteTexture->SetFilterAndWrapMode(GfxFilterMode::Point, GfxWrapMode::Clamp);
        }

        return s_pWhiteTexture.get();
    }

    GfxTexture2D::GfxTexture2D(GfxDevice* device) : GfxTexture(device), m_MetaData{}
    {
    }

    uint32_t GfxTexture2D::GetWidth() const
    {
        return static_cast<uint32_t>(m_MetaData.width);
    }

    uint32_t GfxTexture2D::GetHeight() const
    {
        return static_cast<uint32_t>(m_MetaData.height);
    }

    void GfxTexture2D::LoadFromDDS(const std::string& name, const void* pSourceDDS, uint32_t size)
    {
        ReleaseD3D12Resource();

        // https://github.com/microsoft/DirectXTex/wiki/CreateTexture#directx-12

        ScratchImage image;
        GFX_HR(LoadFromDDSMemory(pSourceDDS, static_cast<size_t>(size), DDS_FLAGS_NONE, &m_MetaData, image));

        ID3D12Device* device = m_Device->GetD3D12Device();
        GFX_HR(CreateTexture(device, m_MetaData, &m_Resource));
        m_State = D3D12_RESOURCE_STATE_COMMON; // CreateTexture 使用的 state
        SetD3D12ResourceName(name);

        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        GFX_HR(PrepareUpload(device, image.GetImages(), image.GetImageCount(), m_MetaData, subresources));

        // upload is implemented by application developer. Here's one solution using <d3dx12.h>
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_Resource, 0, static_cast<UINT>(subresources.size()));

        GfxUploadMemory m = m_Device->AllocateTransientUploadMemory(static_cast<uint32_t>(uploadBufferSize));
        UINT64 mOffset = static_cast<UINT64>(m.GetD3D12ResourceOffset(0));
        UpdateSubresources(m_Device->GetGraphicsCommandList()->GetD3D12CommandList(), m_Resource,
            m.GetD3D12Resource(), mOffset, 0, static_cast<UINT>(subresources.size()), subresources.data());

        device->CreateShaderResourceView(m_Resource, nullptr, GetSrvCpuDescriptorHandle());
    }

    bool GfxRenderTextureDesc::IsCompatibleWith(const GfxRenderTextureDesc& other) const
    {
        return Format == other.Format && Width == other.Width && Height == other.Height && SampleCount == other.SampleCount && SampleQuality == other.SampleQuality;
    }

    GfxRenderTexture::GfxRenderTexture(GfxDevice* device, const std::string& name, const GfxRenderTextureDesc& desc) : GfxTexture(device)
    {
        D3D12_RESOURCE_FLAGS flags;
        D3D12_CLEAR_VALUE clearValue = {};
        DXGI_FORMAT resourceFormat;
        ID3D12Device4* d3d12Device = device->GetD3D12Device();

        if (IsDepthStencilFormat(desc.Format))
        {
            clearValue.Format = desc.Format;
            clearValue.DepthStencil.Depth = GfxSupportInfo::GetFarClipPlaneDepth();
            clearValue.DepthStencil.Stencil = 0;

            resourceFormat = GetDepthStencilResFormat(desc.Format);

            flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            m_State = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        else
        {
            clearValue.Format = desc.Format;
            memcpy(clearValue.Color, Colors::Black, sizeof(clearValue.Color));

            resourceFormat = desc.Format;

            flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            m_State = D3D12_RESOURCE_STATE_COMMON;
        }

        GFX_HR(d3d12Device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Tex2D(resourceFormat,
                static_cast<UINT64>(desc.Width),
                static_cast<UINT>(desc.Height),
                1, 1,
                static_cast<UINT>(desc.SampleCount),
                static_cast<UINT>(desc.SampleQuality),
                flags),
            m_State, &clearValue, IID_PPV_ARGS(&m_Resource)));
        SetD3D12ResourceName(name);

        if (IsDepthStencilFormat(desc.Format))
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsDesc = {};
            dsDesc.Format = desc.Format;
            dsDesc.ViewDimension = desc.SampleCount > 1 ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
            dsDesc.Flags = D3D12_DSV_FLAG_NONE;
            dsDesc.Texture2D.MipSlice = 0;

            m_RtvDsvDescriptorHandle = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            d3d12Device->CreateDepthStencilView(m_Resource, &dsDesc, GetRtvDsvCpuDescriptorHandle());

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = GetDepthStencilSRVFormat(desc.Format);
            srvDesc.ViewDimension = desc.SampleCount > 1 ? D3D12_SRV_DIMENSION_TEXTURE2DMS : D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = -1;
            d3d12Device->CreateShaderResourceView(m_Resource, &srvDesc, GetSrvCpuDescriptorHandle());
        }
        else
        {
            m_RtvDsvDescriptorHandle = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            d3d12Device->CreateRenderTargetView(m_Resource, nullptr, GetRtvDsvCpuDescriptorHandle());
            d3d12Device->CreateShaderResourceView(m_Resource, nullptr, GetSrvCpuDescriptorHandle());
        }
    }

    GfxRenderTexture::GfxRenderTexture(GfxDevice* device, const std::string& name, DXGI_FORMAT format, uint32_t width, uint32_t height, uint32_t sampleCount, uint32_t sampleQuality)
        : GfxRenderTexture(device, name, { format, width, height, sampleCount, sampleQuality }) {}

    GfxRenderTexture::~GfxRenderTexture()
    {
        m_Device->FreeDescriptor(m_RtvDsvDescriptorHandle);
    }

    uint32_t GfxRenderTexture::GetWidth() const
    {
        D3D12_RESOURCE_DESC desc = m_Resource->GetDesc();
        return static_cast<uint32_t>(desc.Width);
    }

    uint32_t GfxRenderTexture::GetHeight() const
    {
        D3D12_RESOURCE_DESC desc = m_Resource->GetDesc();
        return static_cast<uint32_t>(desc.Height);
    }

    GfxRenderTextureDesc GfxRenderTexture::GetDesc() const
    {
        D3D12_RESOURCE_DESC desc = m_Resource->GetDesc();

        GfxRenderTextureDesc result = {};
        result.Format = desc.Format;
        result.Width = static_cast<uint32_t>(desc.Width);
        result.Height = static_cast<uint32_t>(desc.Height);
        result.SampleCount = static_cast<uint32_t>(desc.SampleDesc.Count);
        result.SampleQuality = static_cast<uint32_t>(desc.SampleDesc.Quality);
        return result;
    }

    bool GfxRenderTexture::IsDepthStencilFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return true;

        default:
            return false;
        }
    }

    DXGI_FORMAT GfxRenderTexture::GetDepthStencilResFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32G8X24_TYPELESS;

        default:
            throw std::invalid_argument("Invalid depth stencil format");
        }
    }

    DXGI_FORMAT GfxRenderTexture::GetDepthStencilSRVFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

        default:
            throw std::invalid_argument("Invalid depth stencil format");
        }
    }
}
