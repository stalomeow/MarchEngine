#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxTexture.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxException.h"
#include "Engine/Rendering/D3D12Impl/GfxCommand.h"
#include "Engine/Rendering/D3D12Impl/GfxSettings.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Scripting/DotNetRuntime.h"
#include "Engine/Scripting/DotNetMarshal.h"
#include "Engine/Misc/HashUtils.h"
#include "Engine/Misc/PlatformUtils.h"
#include "Engine/Debug.h"
#include <DirectXColors.h>
//#include <DirectXTexEXR.h>
#include <filesystem>
#include <optional>

using namespace DirectX;
using namespace Microsoft::WRL;
namespace fs = std::filesystem;

namespace march
{
    GfxTexture::GfxTexture(GfxDevice* device)
        : m_Device(device)
        , m_Resource(nullptr)
        , m_Desc{}
        , m_MipLevels{}
        , m_SampleQuality{}
        , m_SrvDescriptors{}
        , m_UavDescriptors{}
        , m_RtvDsvDescriptors{}
        , m_SamplerDescriptor{}
    {
    }

    GfxTexture::~GfxTexture()
    {
        ReleaseResource();
    }

    GfxTexture::GfxTexture(GfxTexture&& other) noexcept
        : m_Device(other.m_Device)
        , m_Resource(std::move(other.m_Resource))
        , m_Desc(other.m_Desc)
        , m_MipLevels(other.m_MipLevels)
        , m_SampleQuality(other.m_SampleQuality)
        , m_SrvDescriptors{}
        , m_UavDescriptors{}
        , m_RtvDsvDescriptors(std::move(other.m_RtvDsvDescriptors))
        , m_SamplerDescriptor(std::move(other.m_SamplerDescriptor))
    {
        for (size_t i = 0; i < std::size(m_SrvDescriptors); i++)
        {
            m_SrvDescriptors[i] = std::move(other.m_SrvDescriptors[i]);
        }

        for (size_t i = 0; i < std::size(m_UavDescriptors); i++)
        {
            m_UavDescriptors[i] = std::move(other.m_UavDescriptors[i]);
        }
    }

    GfxTexture& GfxTexture::operator=(GfxTexture&& other) noexcept
    {
        if (this != &other)
        {
            ReleaseResource();

            m_Device = other.m_Device;
            m_Resource = std::move(other.m_Resource);
            m_Desc = other.m_Desc;
            m_MipLevels = other.m_MipLevels;
            m_SampleQuality = other.m_SampleQuality;

            for (size_t i = 0; i < std::size(m_SrvDescriptors); i++)
            {
                m_SrvDescriptors[i] = std::move(other.m_SrvDescriptors[i]);
            }

            for (size_t i = 0; i < std::size(m_UavDescriptors); i++)
            {
                m_UavDescriptors[i] = std::move(other.m_UavDescriptors[i]);
            }

            m_RtvDsvDescriptors = std::move(other.m_RtvDsvDescriptors);
            m_SamplerDescriptor = std::move(other.m_SamplerDescriptor);
        }

        return *this;
    }

    void GfxTexture::ReleaseResource()
    {
        if (m_Resource)
        {
            m_Device->DeferredRelease(m_Resource);
            m_Resource = nullptr;
        }

        for (std::unordered_map<uint32_t, GfxOfflineDescriptor>& srvMap : m_SrvDescriptors)
        {
            srvMap.clear();
        }

        for (std::unordered_map<uint32_t, GfxOfflineDescriptor>& uavMap : m_UavDescriptors)
        {
            uavMap.clear();
        }

        m_RtvDsvDescriptors.clear();
        m_SamplerDescriptor.reset();
    }

    void GfxTexture::Reset(const GfxTextureDesc& desc, RefCountPtr<GfxResource> resource)
    {
        ReleaseResource();

        m_Desc = desc;
        m_Resource = resource;

        D3D12_RESOURCE_DESC resDesc = m_Resource->GetD3DResourceDesc();
        m_MipLevels = static_cast<uint32_t>(resDesc.MipLevels);
        m_SampleQuality = static_cast<uint32_t>(resDesc.SampleDesc.Quality);
    }

    static size_t GetSrvUavIndex(const GfxTextureDesc& desc, GfxTextureElement element)
    {
        if (desc.IsDepthStencil())
        {
            if (element == GfxTextureElement::Default || element == GfxTextureElement::Depth)
            {
                return 0;
            }
            else if (element == GfxTextureElement::Stencil)
            {
                return 1;
            }
        }
        else if (element == GfxTextureElement::Default || element == GfxTextureElement::Color)
        {
            return 0;
        }

        throw GfxException("Invalid texture element");
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetSrv(GfxTextureElement element, std::optional<uint32_t> mipSlice)
    {
        bool useMSAA = m_Desc.MSAASamples > 1;
        UINT mostDetailedMip = 0;
        UINT mipLevels = -1; // Set to -1 to indicate all the mipmap levels from MostDetailedMip on down to least detailed.

        // 如果有 MSAA 的话，就没有 mip slice
        if (useMSAA)
        {
            mipSlice = 0;
        }
        else if (mipSlice)
        {
            mostDetailedMip = static_cast<UINT>(*mipSlice);
            mipLevels = 1;
        }

        GfxOfflineDescriptor& srv = m_SrvDescriptors[GetSrvUavIndex(m_Desc, element)][mipSlice.value_or(-1)];

        if (!srv)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Format = m_Desc.GetSrvUavDXGIFormat(element);
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            if (useMSAA)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                    srvDesc.Texture2DMSArray.FirstArraySlice = 0;
                    srvDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid srv dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
                    srvDesc.Texture2D.MipLevels = mipLevels;
                    srvDesc.Texture2D.PlaneSlice = m_Desc.GetPlaneSlice(element);
                    srvDesc.Texture2D.ResourceMinLODClamp = 0;
                    break;
                case GfxTextureDimension::Tex3D:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    srvDesc.Texture3D.MostDetailedMip = mostDetailedMip;
                    srvDesc.Texture3D.MipLevels = mipLevels;
                    srvDesc.Texture3D.ResourceMinLODClamp = 0;
                    break;
                case GfxTextureDimension::Cube:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                    srvDesc.TextureCube.MostDetailedMip = mostDetailedMip;
                    srvDesc.TextureCube.MipLevels = mipLevels;
                    srvDesc.TextureCube.ResourceMinLODClamp = 0;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                    srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
                    srvDesc.Texture2DArray.MipLevels = mipLevels;
                    srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
                    srvDesc.Texture2DArray.FirstArraySlice = 0;
                    srvDesc.Texture2DArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    srvDesc.Texture2DArray.PlaneSlice = m_Desc.GetPlaneSlice(element);
                    break;
                case GfxTextureDimension::CubeArray:
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                    srvDesc.TextureCubeArray.MostDetailedMip = mostDetailedMip;
                    srvDesc.TextureCubeArray.MipLevels = mipLevels;
                    srvDesc.TextureCubeArray.ResourceMinLODClamp = 0;
                    srvDesc.TextureCubeArray.First2DArrayFace = 0;
                    srvDesc.TextureCubeArray.NumCubes = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid srv dimension");
                }
            }

            GfxDevice* device = GetDevice();
            srv = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            device->GetD3DDevice4()->CreateShaderResourceView(m_Resource->GetD3DResource(), &srvDesc, srv.GetHandle());
        }

        return srv.GetHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetUav(GfxTextureElement element, uint32_t mipSlice)
    {
        if (!m_Desc.HasFlag(GfxTextureFlags::UnorderedAccess))
        {
            throw GfxException("Texture is not created with UnorderedAccess flag");
        }

        if (IsReadOnly())
        {
            throw GfxException("Can not get UAV for read-only texture");
        }

        bool useMSAA = m_Desc.MSAASamples > 1;

        // 如果有 MSAA 的话，就没有 mip slice
        if (useMSAA)
        {
            mipSlice = 0;
        }

        GfxOfflineDescriptor& uav = m_UavDescriptors[GetSrvUavIndex(m_Desc, element)][mipSlice];

        if (!uav)
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = m_Desc.GetSrvUavDXGIFormat(element);

            if (useMSAA)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Cube:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                    uavDesc.Texture2DMSArray.FirstArraySlice = 0;
                    uavDesc.Texture2DMSArray.ArraySize = 6;
                    break;
                case GfxTextureDimension::Tex2DArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                    uavDesc.Texture2DMSArray.FirstArraySlice = 0;
                    uavDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                case GfxTextureDimension::CubeArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                    uavDesc.Texture2DMSArray.FirstArraySlice = 0;
                    uavDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize) * 6;
                    break;
                default:
                    throw GfxException("Invalid uav dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                    uavDesc.Texture2D.MipSlice = static_cast<UINT>(mipSlice);
                    uavDesc.Texture2D.PlaneSlice = m_Desc.GetPlaneSlice(element);
                    break;
                case GfxTextureDimension::Tex3D:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                    uavDesc.Texture3D.MipSlice = static_cast<UINT>(mipSlice);
                    uavDesc.Texture3D.FirstWSlice = 0;
                    uavDesc.Texture3D.WSize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    break;
                case GfxTextureDimension::Cube:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = static_cast<UINT>(mipSlice);
                    uavDesc.Texture2DArray.FirstArraySlice = 0;
                    uavDesc.Texture2DArray.ArraySize = 6;
                    uavDesc.Texture2DArray.PlaneSlice = m_Desc.GetPlaneSlice(element);
                    break;
                case GfxTextureDimension::Tex2DArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = static_cast<UINT>(mipSlice);
                    uavDesc.Texture2DArray.FirstArraySlice = 0;
                    uavDesc.Texture2DArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize);
                    uavDesc.Texture2DArray.PlaneSlice = m_Desc.GetPlaneSlice(element);
                    break;
                case GfxTextureDimension::CubeArray:
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                    uavDesc.Texture2DArray.MipSlice = static_cast<UINT>(mipSlice);
                    uavDesc.Texture2DArray.FirstArraySlice = 0;
                    uavDesc.Texture2DArray.ArraySize = static_cast<UINT>(m_Desc.DepthOrArraySize) * 6;
                    uavDesc.Texture2DArray.PlaneSlice = m_Desc.GetPlaneSlice(element);
                    break;
                default:
                    throw GfxException("Invalid uav dimension");
                }
            }

            GfxDevice* device = GetDevice();
            uav = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->Allocate();
            device->GetD3DDevice4()->CreateUnorderedAccessView(m_Resource->GetD3DResource(), nullptr, &uavDesc, uav.GetHandle());
        }

        return uav.GetHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetRtvDsv(uint32_t wOrArraySlice, uint32_t wOrArraySize, uint32_t mipSlice)
    {
        if (IsReadOnly())
        {
            throw GfxException("Can not get RTV/DSV for read-only texture");
        }

        RtvDsvQuery query = { wOrArraySlice, wOrArraySize, mipSlice };
        auto [it, isNew] = m_RtvDsvDescriptors.try_emplace(query);

        if (isNew)
        {
            CreateRtvDsv(query, it->second);
        }

        return it->second.GetHandle();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetRtvDsv(GfxCubemapFace face, uint32_t faceCount, uint32_t arraySlice, uint32_t mipSlice)
    {
        uint32_t wOrArraySlice = static_cast<uint32_t>(face) + arraySlice * 6u; // 展开为 Texture2DArray
        uint32_t wOrArraySize = faceCount;
        return GetRtvDsv(wOrArraySlice, wOrArraySize, mipSlice);
    }

    void GfxTexture::CreateRtvDsv(const RtvDsvQuery& query, GfxOfflineDescriptor& rtvDsv)
    {
        GfxDevice* device = GetDevice();
        ID3D12Device4* d3dDevice = device->GetD3DDevice4();

        if (m_Desc.IsDepthStencil())
        {
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = m_Desc.GetRtvDsvDXGIFormat();
            dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

            if (m_Desc.MSAASamples > 1)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
                    dsvDesc.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    dsvDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid depth stencil dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                    dsvDesc.Texture2D.MipSlice = static_cast<UINT>(query.MipSlice);
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                    dsvDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    dsvDesc.Texture2DArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    dsvDesc.Texture2DArray.MipSlice = static_cast<UINT>(query.MipSlice);
                    break;
                default:
                    throw GfxException("Invalid depth stencil dimension");
                }
            }

            rtvDsv = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_DSV)->Allocate();
            d3dDevice->CreateDepthStencilView(m_Resource->GetD3DResource(), &dsvDesc, rtvDsv.GetHandle());
        }
        else
        {
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = m_Desc.GetRtvDsvDXGIFormat();

            if (m_Desc.Dimension == GfxTextureDimension::Tex3D)
            {
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.FirstWSlice = static_cast<UINT>(query.WOrArraySlice);
                rtvDesc.Texture3D.WSize = static_cast<UINT>(query.WOrArraySize);
                rtvDesc.Texture3D.MipSlice = static_cast<UINT>(query.MipSlice);
            }
            else if (m_Desc.MSAASamples > 1)
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                    rtvDesc.Texture2DMSArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    rtvDesc.Texture2DMSArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    break;
                default:
                    throw GfxException("Invalid render target dimension");
                }
            }
            else
            {
                switch (m_Desc.Dimension)
                {
                case GfxTextureDimension::Tex2D:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                    rtvDesc.Texture2D.MipSlice = static_cast<UINT>(query.MipSlice);
                    rtvDesc.Texture2D.PlaneSlice = m_Desc.GetPlaneSlice();
                    break;
                case GfxTextureDimension::Cube:
                case GfxTextureDimension::Tex2DArray:
                case GfxTextureDimension::CubeArray:
                    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                    rtvDesc.Texture2DArray.FirstArraySlice = static_cast<UINT>(query.WOrArraySlice);
                    rtvDesc.Texture2DArray.ArraySize = static_cast<UINT>(query.WOrArraySize);
                    rtvDesc.Texture2DArray.MipSlice = static_cast<UINT>(query.MipSlice);
                    rtvDesc.Texture2DArray.PlaneSlice = m_Desc.GetPlaneSlice();
                    break;
                default:
                    throw GfxException("Invalid render target dimension");
                }
            }

            rtvDsv = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)->Allocate();
            d3dDevice->CreateRenderTargetView(m_Resource->GetD3DResource(), &rtvDesc, rtvDsv.GetHandle());
        }
    }

    // 根据 hash 复用 sampler
    static std::unordered_map<size_t, GfxOfflineDescriptor> g_SamplerCache{};

    D3D12_CPU_DESCRIPTOR_HANDLE GfxTexture::GetSampler()
    {
        if (!m_SamplerDescriptor)
        {
            D3D12_SAMPLER_DESC samplerDesc = {};
            samplerDesc.MipLODBias = m_Desc.MipmapBias;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NONE;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

            if (m_Desc.Filter >= GfxTextureFilterMode::AnisotropicMin && m_Desc.Filter <= GfxTextureFilterMode::AnisotropicMax)
            {
                samplerDesc.MaxAnisotropy = static_cast<UINT>(m_Desc.Filter) - static_cast<UINT>(GfxTextureFilterMode::AnisotropicMin) + 1;
                samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
            }
            else
            {
                samplerDesc.MaxAnisotropy = 1;

                // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_filter
                // Note  If you use different filter types for min versus mag filter,
                // undefined behavior occurs in certain cases where the choice between whether magnification or minification happens is ambiguous.
                // To prevent this undefined behavior, use filter modes that use similar filter operations for both min and mag
                // (or use anisotropic filtering, which avoids the issue as well).
                switch (m_Desc.Filter)
                {
                case GfxTextureFilterMode::Point:
                    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                    break;

                case GfxTextureFilterMode::Bilinear:
                    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
                    break;

                case GfxTextureFilterMode::Trilinear:
                    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
                    break;

                case GfxTextureFilterMode::Shadow:
                    samplerDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
                    samplerDesc.ComparisonFunc = GfxSettings::UseReversedZBuffer ? D3D12_COMPARISON_FUNC_GREATER_EQUAL : D3D12_COMPARISON_FUNC_LESS_EQUAL;
                    break;

                default:
                    throw GfxException("Invalid filter mode");
                }
            }

            switch (m_Desc.Wrap)
            {
            case GfxTextureWrapMode::Repeat:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
                break;

            case GfxTextureWrapMode::Clamp:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
                break;

            case GfxTextureWrapMode::Mirror:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
                break;

            case GfxTextureWrapMode::MirrorOnce:
                samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
                samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
                samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
                break;

            default:
                throw GfxException("Invalid wrap mode");
            }

            DefaultHash hash{};
            hash.Append(samplerDesc);
            auto [it, isNew] = g_SamplerCache.try_emplace(*hash);

            if (isNew)
            {
                GfxDevice* device = GetDevice();
                it->second = device->GetOfflineDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)->Allocate();
                device->GetD3DDevice4()->CreateSampler(&samplerDesc, it->second.GetHandle());

                //LOG_TRACE("Create new sampler");
            }

            m_SamplerDescriptor = it->second.GetHandle();
        }

        return *m_SamplerDescriptor;
    }

    uint32_t GfxTexture::GetSubresourceIndex(GfxTextureElement element, uint32_t wOrArraySlice, uint32_t mipSlice) const
    {
        if (m_Desc.Dimension == GfxTextureDimension::Cube || m_Desc.Dimension == GfxTextureDimension::CubeArray)
        {
            throw GfxException("Use GetSubresourceIndex(GfxTextureElement, GfxCubemapFace, uint32_t, uint32_t) for cubemap");
        }

        if (mipSlice >= m_MipLevels)
        {
            throw GfxException("Mip slice out of range");
        }

        if (wOrArraySlice >= m_Desc.DepthOrArraySize)
        {
            throw GfxException("W or array slice out of range");
        }

        return D3D12CalcSubresource(
            static_cast<UINT>(mipSlice),
            static_cast<UINT>(wOrArraySlice),
            static_cast<UINT>(m_Desc.GetPlaneSlice(element)),
            static_cast<UINT>(m_MipLevels),
            static_cast<UINT>(m_Desc.DepthOrArraySize));
    }

    uint32_t GfxTexture::GetSubresourceIndex(GfxTextureElement element, GfxCubemapFace face, uint32_t arraySlice, uint32_t mipSlice) const
    {
        if (m_Desc.Dimension != GfxTextureDimension::Cube && m_Desc.Dimension != GfxTextureDimension::CubeArray)
        {
            throw GfxException("Use GetSubresourceIndex(GfxTextureElement, uint32_t, uint32_t) for non-cubemap");
        }

        if (mipSlice >= m_MipLevels)
        {
            throw GfxException("Mip slice out of range");
        }

        if (arraySlice >= m_Desc.DepthOrArraySize)
        {
            throw GfxException("Array slice out of range");
        }

        return D3D12CalcSubresource(
            static_cast<UINT>(mipSlice),
            static_cast<UINT>(arraySlice * 6) + static_cast<UINT>(face),
            static_cast<UINT>(m_Desc.GetPlaneSlice(element)),
            static_cast<UINT>(m_MipLevels),
            static_cast<UINT>(m_Desc.DepthOrArraySize * 6));
    }

    GfxTexture* GfxTexture::GetDefault(GfxDefaultTexture texture, GfxTextureDimension dimension)
    {
        cs<GfxDefaultTexture> csTexture{};
        csTexture.assign(texture);
        cs<GfxTextureDimension> csDimension{};
        csDimension.assign(dimension);
        return DotNet::RuntimeInvoke<GfxTexture*>(ManagedMethod::Texture_NativeGetDefault, csTexture, csDimension);
    }

    void GfxTexture::ClearSamplerCache()
    {
        g_SamplerCache.clear();
    }

    GfxExternalTexture::GfxExternalTexture(GfxDevice* device) : GfxTexture(device), m_Name{}, m_Image{} {}

    void GfxExternalTexture::LoadFromPixels(const std::string& name, const GfxTextureDesc& desc, void* pixelsData, size_t pixelsSize, uint32_t mipLevels)
    {
        DXGI_FORMAT format = desc.GetResDXGIFormat();
        size_t width = static_cast<size_t>(desc.Width);
        size_t height = static_cast<size_t>(desc.Height);
        size_t depthOrArraySize = static_cast<size_t>(desc.DepthOrArraySize);

        switch (desc.Dimension)
        {
        case GfxTextureDimension::Tex2D:
        case GfxTextureDimension::Tex2DArray:
            CHECK_HR(m_Image.Initialize2D(format, width, height, depthOrArraySize, static_cast<size_t>(mipLevels), CP_FLAGS_NONE));
            break;
        case GfxTextureDimension::Tex3D:
            CHECK_HR(m_Image.Initialize3D(format, width, height, depthOrArraySize, static_cast<size_t>(mipLevels), CP_FLAGS_NONE));
            break;
        case GfxTextureDimension::Cube:
        case GfxTextureDimension::CubeArray:
            CHECK_HR(m_Image.InitializeCube(format, width, height, depthOrArraySize, static_cast<size_t>(mipLevels), CP_FLAGS_NONE));
            break;
        default:
            throw GfxException("Invalid texture dimension");
        }

        if (m_Image.GetPixelsSize() != pixelsSize)
        {
            throw GfxException("Invalid pixel size");
        }

        memcpy(m_Image.GetPixels(), pixelsData, pixelsSize);

        m_Name = name;
        UploadImage(desc, CREATETEX_DEFAULT);
    }

    static std::optional<DXGI_FORMAT> GetCompressedFormat(const ScratchImage& image, GfxTextureCompression compression)
    {
        // https://docs.unity3d.com/6000.0/Documentation/Manual/texture-choose-format-by-platform.html
        // https://docs.unity3d.com/6000.0/Documentation/Manual/texture-formats-reference.html
        // https://docs.unity3d.com/6000.0/Documentation/Manual/class-TextureImporter-type-specific.html#default-formats

        DXGI_FORMAT format = image.GetMetadata().format;

        if (IsCompressed(format))
        {
            throw GfxException("Texture format is already compressed");
        }

        DXGI_FORMAT result;

        if (FormatDataType(format) == FORMAT_TYPE_FLOAT)
        {
            // HDR
            switch (compression)
            {
            case GfxTextureCompression::NormalQuality:
            case GfxTextureCompression::HighQuality:
            case GfxTextureCompression::LowQuality:
                // TODO 现在 BC6 是在 CPU 上压缩的，太 tm 慢了，根本没法等
                // result = DXGI_FORMAT_BC6H_UF16;
                // break;
                return std::nullopt;
            default:
                throw GfxException("Invalid texture compression");
            }
        }
        else if (HasAlpha(format) && !image.IsAlphaAllOpaque())
        {
            switch (compression)
            {
            case GfxTextureCompression::NormalQuality:
                result = DXGI_FORMAT_BC3_UNORM;
                break;
            case GfxTextureCompression::HighQuality:
                result = DXGI_FORMAT_BC7_UNORM;
                break;
            case GfxTextureCompression::LowQuality:
                result = DXGI_FORMAT_BC3_UNORM;
                break;
            default:
                throw GfxException("Invalid texture compression");
            }
        }
        else
        {
            switch (compression)
            {
            case GfxTextureCompression::NormalQuality:
                result = DXGI_FORMAT_BC1_UNORM;
                break;
            case GfxTextureCompression::HighQuality:
                result = DXGI_FORMAT_BC7_UNORM;
                break;
            case GfxTextureCompression::LowQuality:
                result = DXGI_FORMAT_BC1_UNORM;
                break;
            default:
                throw GfxException("Invalid texture compression");
            }
        }

        if (IsSRGB(format))
        {
            result = MakeSRGB(result);
        }

        return result;
    }

    void GfxExternalTexture::LoadFromFile(const std::string& name, const std::string& filePath, const LoadTextureFileArgs& args)
    {
        GfxTextureDesc desc = {};
        desc.Flags = args.Flags;
        desc.MSAASamples = 1;
        desc.Filter = args.Filter;
        desc.Wrap = args.Wrap;
        desc.MipmapBias = args.MipmapBias;

        std::wstring wFilePath = PlatformUtils::Windows::Utf8ToWide(filePath);
        fs::path ext = fs::path(filePath).extension();

        if (ext == ".dds")
        {
            CHECK_HR(LoadFromDDSFile(wFilePath.c_str(), DDS_FLAGS_NONE, nullptr, m_Image));
        }
        //else if (ext == ".exr")
        //{
        //    CHECK_HR(LoadFromEXRFile(wFilePath.c_str(), nullptr, m_Image));
        //}
        else
        {
            CHECK_HR(LoadFromWICFile(wFilePath.c_str(), WIC_FLAGS_NONE, nullptr, m_Image));
        }

        if (IsCompressed(m_Image.GetMetadata().format))
        {
            ScratchImage decompressed;
            CHECK_HR(Decompress(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), DXGI_FORMAT_UNKNOWN, decompressed));
            m_Image = std::move(decompressed);
        }

        if (desc.HasFlag(GfxTextureFlags::Mipmaps))
        {
            if (m_Image.GetMetadata().mipLevels == 1 && (m_Image.GetMetadata().width > 1 || m_Image.GetMetadata().height > 1))
            {
                ScratchImage mipChain;
                HRESULT hr;

                if (m_Image.GetMetadata().dimension == TEX_DIMENSION_TEXTURE3D)
                {
                    // https://github.com/microsoft/DirectXTex/wiki/GenerateMipMaps3D
                    // This function does not operate directly on block compressed images.
                    hr = GenerateMipMaps3D(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), TEX_FILTER_BOX, 0, mipChain);
                }
                else
                {
                    // https://github.com/microsoft/DirectXTex/wiki/GenerateMipMaps
                    // This function does not operate directly on block compressed images.
                    hr = GenerateMipMaps(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), TEX_FILTER_BOX, 0, mipChain);
                }

                if (SUCCEEDED(hr))
                {
                    m_Image = std::move(mipChain);
                }
                else
                {
                    // 在边长不是 pow2 的情况下会失败
                    desc.Flags &= ~GfxTextureFlags::Mipmaps;
                }
            }
        }
        else if (m_Image.GetMetadata().mipLevels > 1)
        {
            TexMetadata metadata = m_Image.GetMetadata();
            metadata.mipLevels = 1; // Remove mipmaps

            ScratchImage level0;
            CHECK_HR(level0.Initialize(metadata, CP_FLAGS_NONE));

            if (metadata.dimension == TEX_DIMENSION_TEXTURE3D)
            {
                for (size_t i = 0; i < metadata.depth; i++)
                {
                    const Image* src = m_Image.GetImage(0, 0, i);
                    const Image* dst = level0.GetImage(0, 0, i);
                    memcpy(dst->pixels, src->pixels, src->slicePitch);
                }
            }
            else
            {
                for (size_t i = 0; i < metadata.arraySize; i++)
                {
                    const Image* src = m_Image.GetImage(0, i, 0);
                    const Image* dst = level0.GetImage(0, i, 0);
                    memcpy(dst->pixels, src->pixels, src->slicePitch);
                }
            }

            m_Image = std::move(level0);
        }

        if (args.Compression != GfxTextureCompression::None)
        {
            if (std::optional<DXGI_FORMAT> targetFormat = GetCompressedFormat(m_Image, args.Compression))
            {
                ScratchImage compressed;

                // TODO 目前 BC7 压缩速度巨慢
                TEX_COMPRESS_FLAGS flags = TEX_COMPRESS_BC7_QUICK | TEX_COMPRESS_PARALLEL;

                if (!desc.HasFlag(GfxTextureFlags::SRGB))
                {
                    // By default, BC1-3 uses a perceptual weighting.
                    // By using this flag, the perceptual weighting is disabled which can be useful
                    // when using the RGB channels for other data.
                    flags |= TEX_COMPRESS_UNIFORM;
                }

                CHECK_HR(Compress(m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), *targetFormat, flags, TEX_THRESHOLD_DEFAULT, compressed));
                m_Image = std::move(compressed);
            }
        }

        const TexMetadata& metadata = m_Image.GetMetadata();
        desc.SetResDXGIFormat(metadata.format);
        desc.Width = static_cast<uint32_t>(metadata.width);
        desc.Height = static_cast<uint32_t>(metadata.height);

        switch (metadata.dimension)
        {
        case TEX_DIMENSION_TEXTURE2D:
            if (metadata.IsCubemap())
            {
                desc.DepthOrArraySize = static_cast<uint32_t>(metadata.arraySize / 6);
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::CubeArray : GfxTextureDimension::Cube;
            }
            else
            {
                desc.DepthOrArraySize = static_cast<uint32_t>(metadata.arraySize);
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::Tex2DArray : GfxTextureDimension::Tex2D;
            }
            break;
        case TEX_DIMENSION_TEXTURE3D:
            desc.DepthOrArraySize = static_cast<uint32_t>(metadata.depth);
            desc.Dimension = GfxTextureDimension::Tex3D;
            break;
        default:
            throw GfxException("Invalid texture dimension");
        }

        // https://github.com/microsoft/DirectXTex/wiki/CreateTexture
        // The CREATETEX_SRGB flag provides an option for working around gamma issues with content
        // that is in the sRGB or similar color space but is not encoded explicitly as an SRGB format.
        // This will force the resource format be one of the of DXGI_FORMAT_*_SRGB formats if it exist.
        // Note that no pixel data conversion takes place.
        // The CREATETEX_IGNORE_SRGB flag does the opposite;
        // it will force the resource format to not have the _*_SRGB version.
        CREATETEX_FLAGS createFlags;

        if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Linear)
        {
            createFlags = desc.HasFlag(GfxTextureFlags::SRGB) ? CREATETEX_FORCE_SRGB : CREATETEX_IGNORE_SRGB;
        }
        else
        {
            // shader 中采样时不进行任何转换
            createFlags = CREATETEX_IGNORE_SRGB;
        }

        m_Name = name;
        UploadImage(desc, createFlags);
    }

    void GfxExternalTexture::UploadImage(const GfxTextureDesc& desc, CREATETEX_FLAGS flags)
    {
        // https://github.com/microsoft/DirectXTex/wiki/CreateTexture#directx-12

        GfxDevice* device = GetDevice();
        ID3D12Device4* d3dDevice = device->GetD3DDevice4();

        ComPtr<ID3D12Resource> resource = nullptr;
        CHECK_HR(CreateTextureEx(d3dDevice, m_Image.GetMetadata(), desc.GetResFlags(true), flags, resource.GetAddressOf()));
        GfxUtils::SetName(resource.Get(), m_Name);

        // CreateTextureEx 使用 D3D12_RESOURCE_STATE_COMMON
        Reset(desc, MARCH_MAKE_REF(GfxResource, device, resource, D3D12_RESOURCE_STATE_COMMON));

        std::vector<D3D12_SUBRESOURCE_DATA> subresources{};
        CHECK_HR(PrepareUpload(d3dDevice, m_Image.GetImages(), m_Image.GetImageCount(), m_Image.GetMetadata(), subresources));

        GfxCommandContext* context = device->RequestContext(GfxCommandType::Direct);
        context->UpdateSubresources(GetUnderlyingResource(),
            0, static_cast<uint32_t>(subresources.size()), subresources.data());
        context->TransitionResource(GetUnderlyingResource(), D3D12_RESOURCE_STATE_GENERIC_READ); // 方便后续读取，尤其是 AsyncCompute
        context->SubmitAndRelease().WaitOnCpu();

        // 外部导入的资源，始终保持 GENERIC_READ，不允许修改状态
        GetUnderlyingResource()->LockState(true);
    }

    GfxRenderTexture::GfxRenderTexture(GfxDevice* device, const std::string& name, const GfxTextureDesc& desc, GfxTextureAllocStrategy allocationStrategy)
        : GfxTexture(device)
    {
        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Alignment = 0;
        resDesc.Width = static_cast<UINT64>(std::max(1u, desc.Width));
        resDesc.Height = static_cast<UINT>(std::max(1u, desc.Height));
        resDesc.DepthOrArraySize = static_cast<UINT16>(desc.DepthOrArraySize);
        resDesc.Format = desc.GetResDXGIFormat();
        resDesc.SampleDesc.Count = static_cast<UINT>(desc.MSAASamples);
        resDesc.SampleDesc.Quality = static_cast<UINT>(device->GetMSAAQuality(resDesc.Format, resDesc.SampleDesc.Count));
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.Flags = desc.GetResFlags(false);

        if (desc.HasFlag(GfxTextureFlags::Mipmaps))
        {
            // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_resource_desc
            // When 0 is used, the API will automatically calculate the maximum mip levels supported and use that.
            resDesc.MipLevels = 0;
        }
        else
        {
            resDesc.MipLevels = 1;
        }

        switch (desc.Dimension)
        {
        case GfxTextureDimension::Tex2D:
        case GfxTextureDimension::Tex2DArray:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case GfxTextureDimension::Cube:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.DepthOrArraySize = 6u;
            break;
        case GfxTextureDimension::CubeArray:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resDesc.DepthOrArraySize *= 6u;
            break;
        case GfxTextureDimension::Tex3D:
            resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        default:
            throw GfxException("Invalid texture dimension");
        }

        D3D12_CLEAR_VALUE clearValue = {};
        clearValue.Format = desc.GetRtvDsvDXGIFormat();

        D3D12_RESOURCE_STATES initialState;

        if (desc.IsDepthStencil())
        {
            clearValue.DepthStencil.Depth = GfxUtils::FarClipPlaneDepth;
            clearValue.DepthStencil.Stencil = 0;

            initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }
        else
        {
            memcpy(clearValue.Color, Colors::Black, sizeof(clearValue.Color));

            initialState = D3D12_RESOURCE_STATE_COMMON;
        }

        GfxResourceAllocator* allocator;

        switch (allocationStrategy)
        {
        case GfxTextureAllocStrategy::DefaultHeapCommitted:
            allocator = device->GetCommittedAllocator(D3D12_HEAP_TYPE_DEFAULT);
            break;
        case GfxTextureAllocStrategy::DefaultHeapPlaced:
            allocator = device->GetDefaultHeapPlacedTextureAllocator(true, desc.MSAASamples > 1);
            break;
        default:
            throw GfxException("Invalid texture allocation strategy");
        }

        Reset(desc, allocator->Allocate(name, &resDesc, initialState, &clearValue));
    }

    GfxRenderTexture::GfxRenderTexture(GfxDevice* device, ComPtr<ID3D12Resource> resource, const GfxTextureResourceDesc& resDesc)
        : GfxTexture(device)
    {
        D3D12_RESOURCE_DESC d3d12Desc = resource->GetDesc();

        GfxTextureDesc desc{};
        desc.SetResDXGIFormat(d3d12Desc.Format);
        desc.Flags = resDesc.Flags;
        desc.Width = static_cast<uint32_t>(d3d12Desc.Width);
        desc.Height = static_cast<uint32_t>(d3d12Desc.Height);
        desc.DepthOrArraySize = static_cast<uint32_t>(d3d12Desc.DepthOrArraySize);
        desc.MSAASamples = static_cast<uint32_t>(d3d12Desc.SampleDesc.Count);
        desc.Filter = resDesc.Filter;
        desc.Wrap = resDesc.Wrap;
        desc.MipmapBias = resDesc.MipmapBias;

        switch (d3d12Desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (resDesc.IsCube)
            {
                desc.DepthOrArraySize /= 6u;
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::CubeArray : GfxTextureDimension::Cube;
            }
            else
            {
                desc.Dimension = desc.DepthOrArraySize > 1 ? GfxTextureDimension::Tex2DArray : GfxTextureDimension::Tex2D;
            }
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            desc.Dimension = GfxTextureDimension::Tex3D;
            break;
        default:
            throw GfxException("Invalid resource dimension");
        }

        Reset(desc, MARCH_MAKE_REF(GfxResource, device, resource, resDesc.State));
    }
}
