#include "pch.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Rendering/D3D12Impl/ShaderCompute.h"
#include "Engine/Debug.h"
#include <algorithm>
#include <wrl.h>

using namespace Microsoft::WRL;

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

    std::string ComputeShaderKernel::GetProgramTypePreprocessorMacro(size_t programType)
    {
        return "SHADER_STAGE_COMPUTE";
    }

    void ComputeShaderKernel::RecordEntrypointCallback(size_t programType, std::string& entrypoint)
    {
        entrypoint = GetName();
    }

    D3D12_ROOT_SIGNATURE_FLAGS ComputeShaderKernel::GetRootSignatureFlags(const ProgramMatch& m)
    {
        // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_root_signature_flags
        // The value in denying access to shader stages is a minor optimization on some hardware.
        // If, for example, the D3D12_SHADER_VISIBILITY_ALL flag has been set to broadcast the root signature to all shader stages,
        // then denying access can overrule this and save the hardware some work.
        // Alternatively if the shader is so simple that no root signature resources are needed,
        // then denying access could be used here too.

        // D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        // The app is opting in to using the Input Assembler (requiring an input layout that defines a set of vertex buffer bindings).
        // Omitting this flag can result in one root argument space being saved on some hardware.
        // Omit this flag if the Input Assembler is not required, though the optimization is minor.

        constexpr D3D12_ROOT_SIGNATURE_FLAGS flags
            = D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS
            | D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
        return flags;
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

    uint32_t ComputeShader::GetThreadGroupSizeX(size_t kernelIndex) const
    {
        ShaderProgram* program = m_Kernels[kernelIndex]->GetProgram(0, m_KeywordSet.GetKeywords());
        return program->GetThreadGroupSizeX();
    }

    uint32_t ComputeShader::GetThreadGroupSizeY(size_t kernelIndex) const
    {
        ShaderProgram* program = m_Kernels[kernelIndex]->GetProgram(0, m_KeywordSet.GetKeywords());
        return program->GetThreadGroupSizeY();
    }

    uint32_t ComputeShader::GetThreadGroupSizeZ(size_t kernelIndex) const
    {
        ShaderProgram* program = m_Kernels[kernelIndex]->GetProgram(0, m_KeywordSet.GetKeywords());
        return program->GetThreadGroupSizeZ();
    }

    void ComputeShader::GetThreadGroupSize(size_t kernelIndex, uint32_t* pOutX, uint32_t* pOutY, uint32_t* pOutZ) const
    {
        ShaderProgram* program = m_Kernels[kernelIndex]->GetProgram(0, m_KeywordSet.GetKeywords());
        *pOutX = program->GetThreadGroupSizeX();
        *pOutY = program->GetThreadGroupSizeY();
        *pOutZ = program->GetThreadGroupSizeZ();
    }

    ComputeShader::RootSignatureType* ComputeShader::GetRootSignature(size_t kernelIndex) const
    {
        return m_Kernels[kernelIndex]->GetRootSignature(m_KeywordSet.GetKeywords());
    }

    ID3D12PipelineState* ComputeShader::GetPSO(size_t kernelIndex) const
    {
        ComputeShaderKernel* kernel = m_Kernels[kernelIndex].get();
        const ShaderKeywordSet& keywords = m_KeywordSet.GetKeywords();

        size_t hash = kernel->GetProgramMatch(keywords).Hash;
        ComPtr<ID3D12PipelineState>& result = kernel->m_PipelineStates[hash];

        if (result == nullptr)
        {
            D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
            psoDesc.pRootSignature = kernel->GetRootSignature(keywords)->GetD3DRootSignature();

            ShaderProgram* program = kernel->GetProgram(0, keywords);
            psoDesc.CS.pShaderBytecode = program->GetBinaryData();
            psoDesc.CS.BytecodeLength = static_cast<SIZE_T>(program->GetBinarySize());

            ID3D12Device4* device = GetGfxDevice()->GetD3DDevice4();
            CHECK_HR(device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(result.GetAddressOf())));
            GfxUtils::SetName(result.Get(), GetName() + " - " + kernel->GetName());

            LOG_TRACE("Create Compute PSO for '{}' Kernel of '{}' Shader", kernel->GetName(), GetName());
        }

        return result.Get();
    }

    bool ComputeShader::Compile(const std::string& filename, const std::string& source, const std::vector<std::string>& pragmas, std::vector<std::string>& warnings, std::string& error)
    {
        m_KeywordSpace->Clear();
        m_Kernels.clear();

        ShaderCompilationInternalUtils::EnumeratePragmaArgs(pragmas, [this](const std::vector<std::string>& args) -> bool
        {
            if (args.size() > 1 && args[0] == "kernel")
            {
                ComputeShaderKernel* kernel = m_Kernels.emplace_back(std::make_unique<ComputeShaderKernel>()).get();
                kernel->m_Name = args[1];
            }

            return true;
        });

        bool success = true;

        for (std::unique_ptr<ComputeShaderKernel>& kernel : m_Kernels)
        {
            if (!kernel->Compile(m_KeywordSpace.get(), filename, source, pragmas, warnings, error, [](auto) {}))
            {
                m_KeywordSpace->Clear();
                m_Kernels.clear();
                success = false;
                break;
            }
        }

        m_KeywordSet.TransformToSpace(m_KeywordSpace.get());
        return success;
    }
}
