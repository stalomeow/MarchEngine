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
        D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCpuDescriptorHandle() const { return DescriptorManager::GetCpuHandle(m_SamplerDescriptorHandle); }

    private:
        void UpdateSampler() const;

        FilterMode m_FilterMode = FilterMode::Point;
        WrapMode m_WrapMode = WrapMode::Repeat;
        DirectX::TexMetadata m_MetaData{};

        TypedDescriptorHandle m_TextureDescriptorHandle;
        TypedDescriptorHandle m_SamplerDescriptorHandle;

    public:
        static Texture* GetDefaultBlack();
        static Texture* GetDefaultWhite();

    private:
        static Texture* s_pBlackTexture;
        static Texture* s_pWhiteTexture;
    };
}
