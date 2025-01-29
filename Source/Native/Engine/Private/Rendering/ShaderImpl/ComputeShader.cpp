#include "pch.h"
#include "Engine/Rendering/ShaderImpl/ComputeShader.h"
#include <algorithm>

namespace march
{
    D3D12_SHADER_VISIBILITY ComputeShaderKernel::GetShaderVisibility(size_t programType)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_shader_visibility
        // The compute queue always uses D3D12_SHADER_VISIBILITY_ALL because it has only one active stage.
        // The 3D queue can choose values, but if it uses D3D12_SHADER_VISIBILITY_ALL,
        // all shader stages can access whatever is bound at the root signature slot.
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    bool ComputeShaderKernel::GetEntrypointProgramType(const std::string& key, size_t* pOutProgramType)
    {
        return false;
    }

    std::string ComputeShaderKernel::GetTargetProfile(const std::string& shaderModel, size_t programType)
    {
        std::string model = shaderModel;
        std::replace(model.begin(), model.end(), '.', '_');
        return "cs_" + model;
    }

    void ComputeShaderKernel::RecordEntrypointCallback(size_t programType, std::string& entrypoint)
    {
        entrypoint = GetName();
    }

    std::optional<size_t> ComputeShader::FindKernel(const std::string& name) const
    {
        for (size_t i = 0; i < m_Kernels.size(); i++)
        {
            if (m_Kernels[i]->GetName() == name)
            {
                return i;
            }
        }

        return std::nullopt;
    }

    void ComputeShader::GetThreadGroupSize(size_t kernelIndex, uint32_t* pOutX, uint32_t* pOutY, uint32_t* pOutZ)
    {
        ShaderProgram* program = m_Kernels[kernelIndex]->GetProgram(0, m_KeywordSet.GetKeywordSet());
        program->GetThreadGroupSize(pOutX, pOutY, pOutZ);
    }

    bool ComputeShader::Compile(const std::string& filename, const std::string& source, std::vector<std::string>& warnings, std::string& error)
    {
        m_KeywordSpace.Reset();
        m_Kernels.clear();

        ShaderCompilationInternalUtils::EnumeratePragmas(source, [this](const std::vector<std::string>& args) -> bool
        {
            if (args.size() > 1 && args[0] == "kernel")
            {
                ComputeShaderKernel* kernel = m_Kernels.emplace_back(std::make_unique<ComputeShaderKernel>()).get();
                kernel->m_Name = args[1];
            }

            return true;
        });

        for (std::unique_ptr<ComputeShaderKernel>& kernel : m_Kernels)
        {
            if (!kernel->Compile(m_KeywordSpace, filename, source, warnings, error))
            {
                m_KeywordSpace.Reset();
                m_Kernels.clear();
                return false;
            }
        }

        m_KeywordSet.TransformToSpace(m_KeywordSpace);
        return true;
    }
}
