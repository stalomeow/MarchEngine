#include "pch.h"
#include "Engine/Profiling/NsightAftermath.h"
#include "Engine/Misc/DeferFunc.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Application.h"
#include "Engine/Debug.h"
#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <fmt/chrono.h>

namespace fs = std::filesystem;

namespace march
{
    static void GFSDK_AFTERMATH_CALL GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
    static void GFSDK_AFTERMATH_CALL ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
    static void GFSDK_AFTERMATH_CALL CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* pUserData);

    static bool g_IsInitialized = false;

    // Enable Nsight Aftermath GPU crash dump creation.
    // This needs to be done before the D3D device is created.
    void NsightAftermath::InitializeBeforeDeviceCreation()
    {
        if (g_IsInitialized)
        {
            LOG_ERROR("Nsight Aftermath is already initialized.");
            return;
        }

        GFSDK_Aftermath_Result result = GFSDK_Aftermath_EnableGpuCrashDumps(
            GFSDK_Aftermath_Version_API,
            GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
            GpuCrashDumpCallback,
            ShaderDebugInfoCallback,
            CrashDumpDescriptionCallback,
            nullptr,
            nullptr);

        if (!GFSDK_Aftermath_SUCCEED(result))
        {
            LOG_ERROR("Nsight Aftermath failed to initialize ({:X}).", static_cast<uint32_t>(result));
            return;
        }

        g_IsInitialized = true;
    }

    static uint32_t ConvertFeatureFlags(NsightAftermath::FeatureFlags features)
    {
        uint32_t flags = 0;

        if ((features & NsightAftermath::FeatureFlags::EnableMarkers) == NsightAftermath::FeatureFlags::EnableMarkers)
        {
            flags |= GFSDK_Aftermath_FeatureFlags_EnableMarkers;
        }

        if ((features & NsightAftermath::FeatureFlags::EnableResourceTracking) == NsightAftermath::FeatureFlags::EnableResourceTracking)
        {
            flags |= GFSDK_Aftermath_FeatureFlags_EnableResourceTracking;
        }

        if ((features & NsightAftermath::FeatureFlags::CallStackCapturing) == NsightAftermath::FeatureFlags::CallStackCapturing)
        {
            flags |= GFSDK_Aftermath_FeatureFlags_CallStackCapturing;
        }

        if ((features & NsightAftermath::FeatureFlags::GenerateShaderDebugInfo) == NsightAftermath::FeatureFlags::GenerateShaderDebugInfo)
        {
            flags |= GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo;
        }

        if ((features & NsightAftermath::FeatureFlags::EnableShaderErrorReporting) == NsightAftermath::FeatureFlags::EnableShaderErrorReporting)
        {
            flags |= GFSDK_Aftermath_FeatureFlags_EnableShaderErrorReporting;
        }

        return flags;
    }

    void NsightAftermath::InitializeDevice(ID3D12Device* device, FeatureFlags features)
    {
        if (!g_IsInitialized)
        {
            LOG_ERROR("Nsight Aftermath is not initialized. Call InitializeBeforeDeviceCreation() first.");
            return;
        }

        uint32_t aftermathFlags = ConvertFeatureFlags(features);
        GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_Initialize(GFSDK_Aftermath_Version_API, aftermathFlags, device);

        if (result != GFSDK_Aftermath_Result_Success)
        {
            g_IsInitialized = false;
            LOG_ERROR("Nsight Aftermath failed to initialize ({:X}).", static_cast<uint32_t>(result));
            return;
        }

        LOG_INFO("Nsight Aftermath initialized successfully.");
    }

    static std::string GetErrorMessage(GFSDK_Aftermath_Result result)
    {
        switch (result)
        {
        case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
            return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
        case GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported:
            return "Aftermath is incompatible with D3D API interception, such as PIX or Nsight Graphics.";
        default:
            return StringUtils::Format("Aftermath Error 0x{:X}", static_cast<uint32_t>(result));
        }
    }

    static void HandleAftermathError(GFSDK_Aftermath_Result result)
    {
        GetApp()->CrashWithMessage("Nsight Aftermath Error", GetErrorMessage(result), /* debugBreak */ true);
    }

#define AFTERMATH_CHECK_ERROR(FC)          \
{                                          \
    GFSDK_Aftermath_Result _result = FC;   \
    if (!GFSDK_Aftermath_SUCCEED(_result)) \
    {                                      \
        HandleAftermathError(_result);     \
    }                                      \
}

    static std::string GetCrashDumpFilePath()
    {
        fs::path path = StringUtils::Utf8ToUtf16(GetApp()->GetDataPath() + "/Logs");

        if (!fs::exists(path) && !fs::create_directories(path))
        {
            LOG_ERROR("Failed to create directory: {}", path.string());
            return "";
        }

        // Write the crash dump data to a file using the .nv-gpudmp extension registered with Nsight Graphics.
        path /= StringUtils::Format(L"Crash-{:%Y-%m-%d-%H-%M-%S}.nv-gpudmp", fmt::localtime(std::time(nullptr)));
        return path.string();
    }

    static void GFSDK_AFTERMATH_CALL GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
    {
        const std::string crashDumpFileName = GetCrashDumpFilePath();

        if (crashDumpFileName.empty())
        {
            LOG_ERROR("Failed to create crash dump file.");
            return;
        }

        std::ofstream dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);

        if (dumpFile)
        {
            dumpFile.write(static_cast<const char*>(pGpuCrashDump), gpuCrashDumpSize);
            dumpFile.close();
        }

        // Create a GPU crash dump decoder object for the GPU crash dump.
        GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
            GFSDK_Aftermath_Version_API,
            pGpuCrashDump,
            gpuCrashDumpSize,
            &decoder));

        // Destroy the GPU crash dump decoder object.
        DEFER_FUNC() { AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder)); };

        // Decode the crash dump to a JSON string.
        // Step 1: Generate the JSON and get the size.
        uint32_t jsonSize = 0;
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
            decoder,
            GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
            GFSDK_Aftermath_GpuCrashDumpFormatterFlags_UTF8_OUTPUT,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            &jsonSize));

        // Step 2: Allocate a buffer and fetch the generated JSON.
        std::vector<char> json(jsonSize);
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(
            decoder,
            jsonSize,
            json.data()));

        // Write the crash dump data as JSON to a file.
        const std::string jsonFileName = crashDumpFileName + ".json";
        std::ofstream jsonFile(jsonFileName, std::ios::out | std::ios::binary);

        if (jsonFile)
        {
            // Write the JSON to the file (excluding string termination)
            jsonFile.write(json.data(), json.size() - 1);
            jsonFile.close();
        }
    }

    static std::string GetShaderDebugInfoFilePath(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier)
    {
        fs::path path = StringUtils::Utf8ToUtf16(GetApp()->GetDataPath() + "/Logs/DebugInfo");

        if (!fs::exists(path) && !fs::create_directories(path))
        {
            LOG_ERROR("Failed to create directory: {}", path.string());
            return "";
        }

        path /= StringUtils::Format(L"Shader-{:X}-{:X}.nvdbg", identifier.id[0], identifier.id[1]);
        return path.string();
    }

    static void GFSDK_AFTERMATH_CALL ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
    {
        // Get shader debug information identifier
        GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
            GFSDK_Aftermath_Version_API,
            pShaderDebugInfo,
            shaderDebugInfoSize,
            &identifier));

        const std::string filePath = GetShaderDebugInfoFilePath(identifier);
        std::ofstream f(filePath, std::ios::out | std::ios::binary);

        if (f)
        {
            // Write to file for later in-depth analysis of crash dumps with Nsight Graphics
            f.write(static_cast<const char*>(pShaderDebugInfo), shaderDebugInfoSize);
        }
    }

    static void GFSDK_AFTERMATH_CALL CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* pUserData)
    {
        addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, GetApp()->GetProjectName().c_str());
        addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "1.0.0"); // TODO app version
    }

    bool NsightAftermath::OnGpuCrash()
    {
        if (!g_IsInitialized)
        {
            return false;
        }

        // DXGI_ERROR error notification is asynchronous to the NVIDIA display driver's GPU crash handling.
        // Give the Nsight Aftermath GPU crash dump thread some time to do its work before terminating the process.
        auto tdrTerminationTimeout = std::chrono::seconds(10);
        auto tStart = std::chrono::steady_clock::now();
        auto tElapsed = std::chrono::milliseconds::zero();

        GFSDK_Aftermath_CrashDump_Status status = GFSDK_Aftermath_CrashDump_Status_Unknown;
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&status));

        while (status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed &&
            status != GFSDK_Aftermath_CrashDump_Status_Finished &&
            tElapsed < tdrTerminationTimeout)
        {
            // Sleep 50ms and poll the status again until timeout or Aftermath finished processing the crash dump.
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&status));

            auto tEnd = std::chrono::steady_clock::now();
            tElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart);
        }

        return status == GFSDK_Aftermath_CrashDump_Status_Finished;
    }

    // 保证可以用 void* 代替 GFSDK_Aftermath_ResourceHandle
    static_assert(std::is_pointer_v<GFSDK_Aftermath_ResourceHandle>);

    void* NsightAftermath::RegisterResource(ID3D12Resource* resource)
    {
        if (!g_IsInitialized)
        {
            return nullptr;
        }

        GFSDK_Aftermath_ResourceHandle handle = {};
        GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_RegisterResource(resource, &handle);

        if (result == GFSDK_Aftermath_Result_Success)
        {
            return reinterpret_cast<void*>(handle);
        }

        if (result != GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled)
        {
            HandleAftermathError(result);
        }

        return nullptr;
    }

    void NsightAftermath::UnregisterResource(void* resourceHandle)
    {
        if (!g_IsInitialized || !resourceHandle)
        {
            return;
        }

        auto handle = reinterpret_cast<GFSDK_Aftermath_ResourceHandle>(resourceHandle);
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_UnregisterResource(handle));
    }

    // 保证可以用 void* 代替 GFSDK_Aftermath_ContextHandle
    static_assert(std::is_pointer_v<GFSDK_Aftermath_ContextHandle>);

    void* NsightAftermath::CreateContextHandle(ID3D12CommandList* cmdList)
    {
        if (!g_IsInitialized)
        {
            return nullptr;
        }

        GFSDK_Aftermath_ContextHandle handle = {};
        GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_CreateContextHandle(cmdList, &handle);

        if (result == GFSDK_Aftermath_Result_Success)
        {
            return reinterpret_cast<void*>(handle);
        }

        if (result != GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled)
        {
            HandleAftermathError(result);
        }

        return nullptr;
    }

    void NsightAftermath::ReleaseContextHandle(void* contextHandle)
    {
        if (!g_IsInitialized || !contextHandle)
        {
            return;
        }

        auto handle = reinterpret_cast<GFSDK_Aftermath_ContextHandle>(contextHandle);
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_ReleaseContextHandle(handle));
    }

    void NsightAftermath::SetEventMarker(void* contextHandle, const std::string& label)
    {
        if (!g_IsInitialized || !contextHandle)
        {
            return;
        }

        auto handle = reinterpret_cast<GFSDK_Aftermath_ContextHandle>(contextHandle);
        uint32_t dataSize = static_cast<uint32_t>(label.size() + 1); // 需要加上 '\0' 字符
        GFSDK_Aftermath_Result result = GFSDK_Aftermath_SetEventMarker(handle, label.c_str(), dataSize);

        if (result != GFSDK_Aftermath_Result_Success && result != GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled)
        {
            HandleAftermathError(result);
        }
    }
}
