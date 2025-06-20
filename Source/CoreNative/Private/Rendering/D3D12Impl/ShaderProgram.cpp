#include "pch.h"
#include "Engine/Rendering/D3D12Impl/ShaderProgram.h"
#include "Engine/Rendering/D3D12Impl/ShaderUtils.h"
#include "Engine/Rendering/D3D12Impl/GfxSettings.h"
#include "Engine/Rendering/D3D12Impl/GfxDevice.h"
#include "Engine/Rendering/D3D12Impl/GfxUtils.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Misc/PlatformUtils.h"
#include "Engine/Application.h"
#include "Engine/Debug.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <sstream>
#include <functional>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <assert.h>

using namespace Microsoft::WRL;
namespace fs = std::filesystem;

namespace march
{
    // ID3D12RootSignature 根据 Hash 复用
    static std::unordered_map<size_t, ComPtr<ID3D12RootSignature>> g_GlobalRootSignaturePool{};

    void ShaderUtils::ClearRootSignatureCache()
    {
        g_GlobalRootSignaturePool.clear();
    }

    void ShaderRootSignatureInternalUtils::AddStaticSamplers(
        std::vector<CD3DX12_STATIC_SAMPLER_DESC>& samplers,
        ShaderProgram* program,
        D3D12_SHADER_VISIBILITY visibility)
    {
        static std::unordered_map<int32, CD3DX12_STATIC_SAMPLER_DESC> cache{};

        // build cache
        if (cache.empty())
        {
            auto filters =
            {
                std::pair{ "Point"    , D3D12_FILTER_MIN_MAG_MIP_POINT        },
                std::pair{ "Linear"   , D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT },
                std::pair{ "Trilinear", D3D12_FILTER_MIN_MAG_MIP_LINEAR       },
            };

            auto wraps =
            {
                std::pair{ "Repeat"    , D3D12_TEXTURE_ADDRESS_MODE_WRAP        },
                std::pair{ "Clamp"     , D3D12_TEXTURE_ADDRESS_MODE_CLAMP       },
                std::pair{ "Mirror"    , D3D12_TEXTURE_ADDRESS_MODE_MIRROR      },
                std::pair{ "MirrorOnce", D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE },
            };

            using namespace std::string_literals;

            for (const auto& [filterName, filterValue] : filters)
            {
                for (const auto& [wrapName, wrapValue] : wraps)
                {
                    std::string name = "sampler_"s + filterName + wrapName;
                    CD3DX12_STATIC_SAMPLER_DESC& desc = cache[ShaderUtils::GetIdFromString(name)];

                    desc.Filter = filterValue;
                    desc.AddressU = wrapValue;
                    desc.AddressV = wrapValue;
                    desc.AddressW = wrapValue;
                }
            }

            // Anisotropic
            for (UINT aniso = 1; aniso <= 16; aniso++)
            {
                for (const auto& [wrapName, wrapValue] : wraps)
                {
                    std::string name = "sampler_Aniso"s + std::to_string(aniso) + wrapName;
                    CD3DX12_STATIC_SAMPLER_DESC& desc = cache[ShaderUtils::GetIdFromString(name)];

                    desc.Filter = D3D12_FILTER_ANISOTROPIC;
                    desc.AddressU = wrapValue;
                    desc.AddressV = wrapValue;
                    desc.AddressW = wrapValue;
                    desc.MaxAnisotropy = aniso;
                }
            }
        }

        for (const ShaderProgramStaticSampler& s : program->GetStaticSamplers())
        {
            if (auto it = cache.find(s.Id); it != cache.end())
            {
                CD3DX12_STATIC_SAMPLER_DESC& desc = samplers.emplace_back(it->second);

                desc.ShaderRegister = static_cast<UINT>(s.ShaderRegister);
                desc.RegisterSpace = static_cast<UINT>(s.RegisterSpace);
                desc.ShaderVisibility = visibility;
            }
        }
    }

    ComPtr<ID3D12RootSignature> ShaderRootSignatureInternalUtils::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& desc)
    {
        ComPtr<ID3DBlob> serializedData = nullptr;
        ComPtr<ID3DBlob> error = nullptr;
        HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, serializedData.GetAddressOf(), error.GetAddressOf());

        if (error != nullptr)
        {
            LOG_ERROR("{}", reinterpret_cast<char*>(error->GetBufferPointer()));
        }

        CHECK_HR(hr);

        LPVOID bufferPointer = serializedData->GetBufferPointer();
        SIZE_T bufferSize = serializedData->GetBufferSize();

        if (bufferSize % 4 != 0)
        {
            throw GfxException("Invalid root signature data size");
        }

        DefaultHash hash{};
        hash.Append(bufferPointer, bufferSize);
        ComPtr<ID3D12RootSignature>& result = g_GlobalRootSignaturePool[*hash];

        if (result == nullptr)
        {
            //LOG_TRACE("Create new RootSignature");

            ID3D12Device4* device = GetGfxDevice()->GetD3DDevice4();
            CHECK_HR(device->CreateRootSignature(0, bufferPointer, bufferSize, IID_PPV_ARGS(result.GetAddressOf())));
        }
        else
        {
            //LOG_TRACE("Reuse RootSignature");
        }

        return result;
    }

    bool ShaderCompilationInternalUtils::EnumeratePragmaArgs(const std::vector<std::string>& pragmas, const std::function<bool(const std::vector<std::string>&)>& fn)
    {
        std::vector<std::string> args{};

        for (const std::string& pragma : pragmas)
        {
            args.clear();
            std::istringstream iss(pragma);

            std::string temp{};
            while (iss >> temp)
            {
                args.emplace_back(std::move(temp));
            }

            if (!args.empty() && !fn(args))
            {
                return false;
            }
        }

        return true;
    }

    void ShaderCompilationInternalUtils::AppendEngineMacros(std::vector<std::wstring>& m)
    {
        if constexpr (GfxSettings::UseReversedZBuffer)
        {
            m.push_back(L"MARCH_REVERSED_Z=1");
        }

        if constexpr (GfxSettings::ColorSpace == GfxColorSpace::Gamma)
        {
            m.push_back(L"MARCH_COLORSPACE_GAMMA=1");
        }

        m.push_back(L"MARCH_NEAR_CLIP_VALUE=" + std::to_wstring(GfxUtils::NearClipPlaneDepth));
        m.push_back(L"MARCH_FAR_CLIP_VALUE=" + std::to_wstring(GfxUtils::FarClipPlaneDepth));

        m.push_back(L"MARCH_SHADER_PROPERTIES");
    }

    void ShaderCompilationInternalUtils::SaveCompilationResults(
        IDxcUtils* utils,
        ComPtr<IDxcResult> pResults,
        ShaderProgram* program,
        const std::function<void(ID3D12ShaderReflectionConstantBuffer*)>& recordConstantBufferCallback)
    {
        // 编译结果
        CHECK_HR(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&program->m_Binary), nullptr));

        // PDB
        ComPtr<IDxcBlob> pPDB = nullptr;
        CHECK_HR(pResults->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(&pPDB), nullptr));

        // Hash
        ComPtr<IDxcBlob> hash = nullptr;
        CHECK_HR(pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&hash), nullptr));
        program->m_Hash.SetData(*static_cast<DxcShaderHash*>(hash->GetBufferPointer()));

        // 保存到 ShaderCache 中
        SaveShaderBinaryAndPdbByHash(program->GetHash(), program->m_Binary.Get(), pPDB.Get());

        // 反射
        ComPtr<IDxcBlob> pReflectionData = nullptr;
        CHECK_HR(pResults->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&pReflectionData), nullptr));

        if (pReflectionData != nullptr)
        {
            // Create reflection interface.
            DxcBuffer ReflectionData{};
            ReflectionData.Encoding = DXC_CP_ACP;
            ReflectionData.Ptr = pReflectionData->GetBufferPointer();
            ReflectionData.Size = pReflectionData->GetBufferSize();

            ComPtr<ID3D12ShaderReflection> pReflection = nullptr;
            CHECK_HR(utils->CreateReflection(&ReflectionData, IID_PPV_ARGS(&pReflection)));

            UINT threadGroupSize[3]{};
            pReflection->GetThreadGroupSize(&threadGroupSize[0], &threadGroupSize[1], &threadGroupSize[2]);
            program->m_ThreadGroupSizeX = static_cast<uint32_t>(threadGroupSize[0]);
            program->m_ThreadGroupSizeY = static_cast<uint32_t>(threadGroupSize[1]);
            program->m_ThreadGroupSizeZ = static_cast<uint32_t>(threadGroupSize[2]);

            D3D12_SHADER_DESC shaderDesc{};
            CHECK_HR(pReflection->GetDesc(&shaderDesc));

            std::unordered_map<int32, ShaderProgramStaticSampler> samplers{};

            // 记录所有资源
            for (UINT i = 0; i < shaderDesc.BoundResources; i++)
            {
                D3D12_SHADER_INPUT_BIND_DESC bindDesc{};
                CHECK_HR(pReflection->GetResourceBindingDesc(i, &bindDesc));

                // TODO rt acceleration structure
                // TODO uav readback texture

                switch (bindDesc.Type)
                {
                case D3D_SIT_CBUFFER:
                {
                    ID3D12ShaderReflectionConstantBuffer* cb = pReflection->GetConstantBufferByName(bindDesc.Name);
                    D3D12_SHADER_BUFFER_DESC cbDesc{};
                    CHECK_HR(cb->GetDesc(&cbDesc));

                    ShaderProgramBuffer& buffer = program->m_SrvCbvBuffers.emplace_back();
                    buffer.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                    buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                    buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                    buffer.IsConstantBuffer = true;
                    recordConstantBufferCallback(cb);
                    break;
                }

                // TODO: tbuffer 怎么用
                // TODO: 如何绑定 Buffer<T>；Buffer<T> 算是 tbuffer 吗
                // https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/sm5-object-buffer
                case D3D_SIT_TBUFFER:
                case D3D_SIT_STRUCTURED:
                case D3D_SIT_BYTEADDRESS:
                {
                    ShaderProgramBuffer& buffer = program->m_SrvCbvBuffers.emplace_back();
                    buffer.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                    buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                    buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                    buffer.IsConstantBuffer = false;
                    break;
                }

                case D3D_SIT_UAV_RWSTRUCTURED:
                case D3D_SIT_UAV_RWBYTEADDRESS:
                case D3D_SIT_UAV_APPEND_STRUCTURED:
                case D3D_SIT_UAV_CONSUME_STRUCTURED:
                case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                {
                    ShaderProgramBuffer& buffer = program->m_UavBuffers.emplace_back();
                    buffer.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                    buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                    buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                    buffer.IsConstantBuffer = false;
                    break;
                }

                case D3D_SIT_TEXTURE:
                {
                    ShaderProgramTexture& tex = program->m_SrvTextures.emplace_back();
                    tex.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                    tex.ShaderRegisterTexture = static_cast<uint32_t>(bindDesc.BindPoint);
                    tex.RegisterSpaceTexture = static_cast<uint32_t>(bindDesc.Space);
                    tex.HasSampler = false; // 先假设没有 sampler
                    tex.ShaderRegisterSampler = 0;
                    tex.RegisterSpaceSampler = 0;
                    break;
                }

                case D3D_SIT_SAMPLER:
                {
                    // 先假设全是 static sampler
                    ShaderProgramStaticSampler sampler{};
                    sampler.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                    sampler.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                    sampler.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                    samplers[sampler.Id] = sampler;
                    break;
                }

                // https://learn.microsoft.com/en-us/windows/win32/api/d3dcommon/ne-d3dcommon-d3d_shader_input_type
                // The shader resource is a read-and-write buffer or texture.
                case D3D_SIT_UAV_RWTYPED:
                {
                    bool isTexture;

                    switch (bindDesc.Dimension)
                    {
                    case D3D_SRV_DIMENSION_TEXTURE1D:
                    case D3D_SRV_DIMENSION_TEXTURE1DARRAY:
                    case D3D_SRV_DIMENSION_TEXTURE2D:
                    case D3D_SRV_DIMENSION_TEXTURE2DARRAY:
                    case D3D_SRV_DIMENSION_TEXTURE2DMS:
                    case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY:
                    case D3D_SRV_DIMENSION_TEXTURE3D:
                    case D3D_SRV_DIMENSION_TEXTURECUBE:
                    case D3D_SRV_DIMENSION_TEXTURECUBEARRAY:
                        isTexture = true;
                        break;
                    default:
                        isTexture = false;
                        break;
                    }

                    if (isTexture)
                    {
                        ShaderProgramTexture& tex = program->m_UavTextures.emplace_back();
                        tex.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                        tex.ShaderRegisterTexture = static_cast<uint32_t>(bindDesc.BindPoint);
                        tex.RegisterSpaceTexture = static_cast<uint32_t>(bindDesc.Space);
                        tex.HasSampler = false; // uav 没有 sampler
                        tex.ShaderRegisterSampler = 0;
                        tex.RegisterSpaceSampler = 0;
                    }
                    else
                    {
                        ShaderProgramBuffer& buffer = program->m_UavBuffers.emplace_back();
                        buffer.Id = ShaderUtils::GetIdFromString(bindDesc.Name);
                        buffer.ShaderRegister = static_cast<uint32_t>(bindDesc.BindPoint);
                        buffer.RegisterSpace = static_cast<uint32_t>(bindDesc.Space);
                        buffer.IsConstantBuffer = false;
                    }

                    break;
                }

                case D3D_SIT_RTACCELERATIONSTRUCTURE:
                case D3D_SIT_UAV_FEEDBACKTEXTURE:
                    break;
                }
            }

            // 记录 texture sampler
            for (ShaderProgramTexture& tex : program->m_SrvTextures)
            {
                int32 samplerId = ShaderUtils::GetIdFromString("sampler" + ShaderUtils::GetStringFromId(tex.Id));

                if (auto it = samplers.find(samplerId); it != samplers.end())
                {
                    tex.HasSampler = true;
                    tex.ShaderRegisterSampler = it->second.ShaderRegister;
                    tex.RegisterSpaceSampler = it->second.RegisterSpace;
                    samplers.erase(it); // 移除假的 static sampler
                }
            }

            // 剩下的就是真的 static sampler
            for (const auto& [_, sampler] : samplers)
            {
                program->m_StaticSamplers.emplace_back(sampler);
            }
        }
    }

    static std::string GetShaderProgramDebugName(const ShaderProgramHash& hash)
    {
        // 所谓的 Shader Debug Name 就是 Shader Hash 转成的 16 进制字符串

        std::ostringstream ss{};

        for (uint8_t b : hash.Data)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }

        return ss.str();
    }

    static std::string GetShaderProgramDebugName(const std::vector<uint8_t>& hash)
    {
        assert(hash.size() == (sizeof(ShaderProgramHash::Data) / sizeof(uint8_t)));

        // 所谓的 Shader Debug Name 就是 Shader Hash 转成的 16 进制字符串

        std::ostringstream ss{};

        for (uint8_t b : hash)
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
        }

        return ss.str();
    }

    static std::string GetShaderCacheBasePath(const std::string& debugName, bool createIfNotExist)
    {
        std::string path = StringUtils::Format("{}/{}{}", GetApp()->GetShaderCachePath(), debugName[0], debugName[1]);

        if (createIfNotExist && !fs::exists(path))
        {
            fs::create_directories(path);
        }

        return path;
    }

    void ShaderCompilationInternalUtils::LoadShaderBinaryByHash(const ShaderProgramHash& hash, IDxcBlob** ppBlob)
    {
        const std::string debugName = GetShaderProgramDebugName(hash);
        const std::string basePath = GetShaderCacheBasePath(debugName, /* createIfNotExist */ false);

        std::wstring path = PlatformUtils::Windows::Utf8ToWide(StringUtils::Format("{}/{}.cso", basePath, debugName));
        CHECK_HR(ShaderUtils::GetDxcUtils()->LoadFile(path.c_str(), DXC_CP_ACP, reinterpret_cast<IDxcBlobEncoding**>(ppBlob)));
    }

    void ShaderCompilationInternalUtils::SaveShaderBinaryAndPdbByHash(const ShaderProgramHash& hash, IDxcBlob* pBinary, IDxcBlob* pPdb)
    {
        const std::string debugName = GetShaderProgramDebugName(hash);
        const std::string basePath = GetShaderCacheBasePath(debugName, /* createIfNotExist */ true);

        // Binary
        {
            std::string path = StringUtils::Format("{}/{}.cso", basePath, debugName);
            std::ofstream fs(path, std::ios::out | std::ios::binary);
            fs.write(reinterpret_cast<const char*>(pBinary->GetBufferPointer()), pBinary->GetBufferSize());
        }

        // PDB
        {
            // pdb 的名称必须和编译器生成的默认名称保持一致
            std::string path = StringUtils::Format("{}/{}.pdb", basePath, debugName);
            std::ofstream fs(path, std::ios::out | std::ios::binary);
            fs.write(reinterpret_cast<const char*>(pPdb->GetBufferPointer()), pPdb->GetBufferSize());
        }
    }

    bool ShaderUtils::HasCachedShaderProgram(const std::vector<uint8_t>& hash)
    {
        const std::string debugName = GetShaderProgramDebugName(hash);
        const std::string basePath = GetShaderCacheBasePath(debugName, /* createIfNotExist */ false);

        return fs::exists(fs::u8path(StringUtils::Format("{}/{}.cso", basePath, debugName)));
    }

    void ShaderUtils::DeleteCachedShaderProgram(const std::vector<uint8_t>& hash)
    {
        const std::string debugName = GetShaderProgramDebugName(hash);
        const std::string basePath = GetShaderCacheBasePath(debugName, /* createIfNotExist */ false);

        // Binary
        {
            fs::remove(fs::u8path(StringUtils::Format("{}/{}.cso", basePath, debugName)));
        }

        // PDB
        {
            fs::remove(fs::u8path(StringUtils::Format("{}/{}.pdb", basePath, debugName)));
        }
    }
}
