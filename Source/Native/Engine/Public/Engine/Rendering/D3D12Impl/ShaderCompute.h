#pragma once

#include "Engine/Ints.h"
#include "Engine/Object.h"
#include "Engine/Rendering/D3D12Impl/ShaderKeyword.h"
#include "Engine/Rendering/D3D12Impl/ShaderProgram.h"
#include <directx/d3dx12.h>
#include <vector>
#include <string>
#include <memory>
#include <optional>

namespace march
{
    class ComputeShaderKernel final : public ShaderProgramGroup<1>
    {
        friend class ComputeShader;
        friend struct ComputeShaderBinding;

    protected:
        D3D12_SHADER_VISIBILITY GetShaderVisibility(size_t programType) override;
        bool GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType) override;
        std::string GetTargetProfile(const std::string& shaderModel, size_t programType) override;
        std::string GetProgramTypePreprocessorMacro(size_t programType) override;
        void RecordEntrypointCallback(size_t programType, std::string& entrypoint) override;
    };

    class ComputeShader : public MarchObject
    {
        friend struct ComputeShaderBinding;

    public:
        static constexpr size_t NumProgramTypes = ComputeShaderKernel::NumProgramTypes;
        using RootSignatureType = ComputeShaderKernel::RootSignatureType;

    private:
        std::string m_Name{};
        std::unique_ptr<ShaderKeywordSpace> m_KeywordSpace = std::make_unique<ShaderKeywordSpace>();
        DynamicShaderKeywordSet m_KeywordSet{}; // 不会被序列化
        std::vector<std::unique_ptr<ComputeShaderKernel>> m_Kernels{};

    public:
        ComputeShader() { m_KeywordSet.TransformToSpace(m_KeywordSpace.get()); }

        const std::string& GetName() const { return m_Name; }

        const ShaderKeywordSpace* GetKeywordSpace() const { return m_KeywordSpace.get(); }

        const ShaderKeywordSet& GetKeywords() const { return m_KeywordSet.GetKeywords(); }

        void EnableKeyword(const std::string& keyword) { m_KeywordSet.EnableKeyword(keyword); }

        void EnableKeyword(int32 keywordId) { m_KeywordSet.EnableKeyword(keywordId); }

        void DisableKeyword(const std::string& keyword) { m_KeywordSet.DisableKeyword(keyword); }

        void DisableKeyword(int32 keywordId) { m_KeywordSet.DisableKeyword(keywordId); }

        void SetKeyword(const std::string& keyword, bool value) { m_KeywordSet.SetKeyword(keyword, value); }

        void SetKeyword(int32 keywordId, bool value) { m_KeywordSet.SetKeyword(keywordId, value); }

        size_t GetKernelCount() const { return m_Kernels.size(); }

        std::optional<size_t> FindKernel(const std::string& name) const;

        uint32_t GetThreadGroupSizeX(size_t kernelIndex) const;
        uint32_t GetThreadGroupSizeY(size_t kernelIndex) const;
        uint32_t GetThreadGroupSizeZ(size_t kernelIndex) const;
        void GetThreadGroupSize(size_t kernelIndex, uint32_t* pOutX, uint32_t* pOutY, uint32_t* pOutZ) const;

        RootSignatureType* GetRootSignature(size_t kernelIndex) const;

        ID3D12PipelineState* GetPSO(size_t kernelIndex) const;

    private:
        bool Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error);
    };
}
