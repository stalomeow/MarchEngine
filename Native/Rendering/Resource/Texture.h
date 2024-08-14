#pragma once

#include "Rendering/Resource/GpuResource.h"
#include "Rendering/DescriptorHeap.h"
#include "Scripting/ScriptTypes.h"
#include <DirectXTex.h>
#include <string>

namespace dx12demo
{
    enum class FilterMode : CSharpInt
    {
        Point = 0,
        Bilinear = 1,
        Trilinear = 2,
    };

    enum class WrapMode : CSharpInt
    {
        Repeat = 0,
        Clamp = 1,
        Mirror = 2,
    };

    class Texture : public GpuResource
    {
    public:
        Texture();
        ~Texture();

        void SetDDSData(const std::wstring& name, const void* pSourceDDS, size_t size);

        void SetFilterMode(FilterMode mode)
        {
            m_FilterMode = mode;
            UpdateSampler();
        }

        void SetWrapMode(WrapMode mode)
        {
            m_WrapMode = mode;
            UpdateSampler();
        }

        void SetFilterAndWrapMode(FilterMode filterMode, WrapMode wrapMode)
        {
            m_FilterMode = filterMode;
            m_WrapMode = wrapMode;
            UpdateSampler();
        }

        FilterMode GetFilterMode() const { return m_FilterMode; }
        WrapMode GetWrapMode() const { return m_WrapMode; }

        const DirectX::TexMetadata& GetMetaData() const { return m_MetaData; }

        D3D12_CPU_DESCRIPTOR_HANDLE GetTextureCpuDescriptorHandle() const { return DescriptorManager::GetCpuHandle(m_TextureDescriptorHandle); }
        D3D12_GPU_DESCRIPTOR_HANDLE GetTextureGpuDescriptorHandle() const { return DescriptorManager::GetGpuHandle(m_TextureDescriptorHandle); }
        
        D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCpuDescriptorHandle() const { return DescriptorManager::GetCpuHandle(m_SamplerDescriptorHandle); }
        D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGpuDescriptorHandle() const { return DescriptorManager::GetGpuHandle(m_SamplerDescriptorHandle); }

    private:
        void UpdateSampler() const;

        FilterMode m_FilterMode = FilterMode::Point;
        WrapMode m_WrapMode = WrapMode::Repeat;
        DirectX::TexMetadata m_MetaData{};

        DescriptorHandle m_TextureDescriptorHandle;
        DescriptorHandle m_SamplerDescriptorHandle;
    };

    namespace binding
    {
        inline CSHARP_API(Texture*) Texture_New()
        {
            return new Texture();
        }

        inline CSHARP_API(void) Texture_Delete(Texture* pTexture)
        {
            delete pTexture;
        }

        inline CSHARP_API(void) Texture_SetDDSData(Texture* pTexture, CSharpString name, const void* pSourceDDS, CSharpInt size)
        {
            pTexture->SetDDSData(CSharpString_ToUtf16(name), pSourceDDS, static_cast<size_t>(size));
        }

        inline CSHARP_API(void) Texture_SetFilterMode(Texture* pTexture, FilterMode mode)
        {
            pTexture->SetFilterMode(mode);
        }

        inline CSHARP_API(void) Texture_SetWrapMode(Texture* pTexture, WrapMode mode)
        {
            pTexture->SetWrapMode(mode);
        }

        inline CSHARP_API(FilterMode) Texture_GetFilterMode(Texture* pTexture)
        {
            return pTexture->GetFilterMode();
        }

        inline CSHARP_API(WrapMode) Texture_GetWrapMode(Texture* pTexture)
        {
            return pTexture->GetWrapMode();
        }
    }
}
