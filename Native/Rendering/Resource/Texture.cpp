#include <directx/d3dx12.h>
#include <d3d12.h>
#include "Rendering/Resource/Texture.h"
#include "Rendering/DxException.h"
#include "Rendering/Command/CommandBuffer.h"
#include <vector>

using namespace DirectX;

namespace dx12demo
{
    Texture::Texture()
    {
        m_TextureDescriptorHandle = DescriptorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_SamplerDescriptorHandle = DescriptorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        UpdateSampler();
    }

    Texture::~Texture()
    {
        DescriptorManager::Free(m_TextureDescriptorHandle);
        DescriptorManager::Free(m_SamplerDescriptorHandle);
    }

    void Texture::SetDDSData(const std::wstring& name, const void* pSourceDDS, size_t size)
    {
        if (m_Resource != nullptr)
        {
            GetGfxManager().SafeReleaseResource(m_Resource);
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

        device->CreateShaderResourceView(m_Resource, nullptr, DescriptorManager::GetCpuHandle(m_TextureDescriptorHandle));
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
        case dx12demo::WrapMode::Repeat:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
            break;
        case dx12demo::WrapMode::Clamp:
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            break;
        case dx12demo::WrapMode::Mirror:
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
        device->CreateSampler(&samplerDesc, DescriptorManager::GetCpuHandle(m_SamplerDescriptorHandle));
    }
}
