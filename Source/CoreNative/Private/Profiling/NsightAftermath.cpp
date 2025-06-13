#include "pch.h"
#include "Engine/Profiling/NsightAftermath.h"
#include "Engine/Misc/StringUtils.h"
#include "Engine/Misc/TimeUtils.h"
#include "Engine/Application.h"
#include "Engine/Debug.h"
#include "GFSDK_Aftermath.h"
#include "GFSDK_Aftermath_GpuCrashDump.h"
#include "GFSDK_Aftermath_GpuCrashDumpDecoding.h"
#include <type_traits>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fmt/chrono.h>

namespace fs = std::filesystem;

// 代码参考 NVIDIA 的官方示例

#define AFTERMATH_CHECK_ERROR(FC)          \
{                                          \
    GFSDK_Aftermath_Result _result = FC;   \
    if (!GFSDK_Aftermath_SUCCEED(_result)) \
    {                                      \
        HandleAftermathError(_result);     \
    }                                      \
}

namespace march
{
    // TODO 目前 EnableMarkers 和 CallStackCapturing 似乎没用
    static constexpr uint32_t FullFeatureFlags
        = GFSDK_Aftermath_FeatureFlags_Minimum
        | GFSDK_Aftermath_FeatureFlags_EnableMarkers
        | GFSDK_Aftermath_FeatureFlags_EnableResourceTracking
        | GFSDK_Aftermath_FeatureFlags_CallStackCapturing
        | GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo
        | GFSDK_Aftermath_FeatureFlags_EnableShaderErrorReporting;

    static void GFSDK_AFTERMATH_CALL GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData);
    static void GFSDK_AFTERMATH_CALL ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData);
    static void GFSDK_AFTERMATH_CALL CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* pUserData);
    static void GFSDK_AFTERMATH_CALL ResolveMarkerCallback(const void* pMarkerData, const uint32_t markerDataSize, void* pUserData, void** ppResolvedMarkerData, uint32_t* pResolvedMarkerDataSize);

    static uint32_t g_FeatureFlags;
    static bool g_IsInitialized = false;

    NsightAftermathState NsightAftermath::GetState()
    {
        if (!g_IsInitialized)
        {
            return NsightAftermathState::Uninitialized;
        }

        return g_FeatureFlags == FullFeatureFlags ? NsightAftermathState::FullFeatures : NsightAftermathState::MinimalFeatures;
    }

    void NsightAftermath::InitializeBeforeDeviceCreation(bool fullFeatures)
    {
        if (g_IsInitialized)
        {
            LOG_ERROR("Nsight Aftermath is already initialized");
            return;
        }

        // Enable Nsight Aftermath GPU crash dump creation.
        // This needs to be done before the D3D device is created.

        GFSDK_Aftermath_Result result = GFSDK_Aftermath_EnableGpuCrashDumps(
            GFSDK_Aftermath_Version_API,
            GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
            GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
            GpuCrashDumpCallback,
            ShaderDebugInfoCallback,
            CrashDumpDescriptionCallback,
            ResolveMarkerCallback,
            nullptr);

        if (!GFSDK_Aftermath_SUCCEED(result))
        {
            LOG_ERROR("Nsight Aftermath failed to initialize before device creation ({:X})", static_cast<uint32_t>(result));
            return;
        }

        g_FeatureFlags = fullFeatures ? FullFeatureFlags : GFSDK_Aftermath_FeatureFlags_Minimum;
        g_IsInitialized = true;
    }

    void NsightAftermath::InitializeDevice(ID3D12Device* device)
    {
        if (!g_IsInitialized)
        {
            LOG_ERROR("Nsight Aftermath is not initialized before device creation");
            return;
        }

        GFSDK_Aftermath_Result result = GFSDK_Aftermath_DX12_Initialize(
            GFSDK_Aftermath_Version_API,
            g_FeatureFlags,
            device);

        if (!GFSDK_Aftermath_SUCCEED(result))
        {
            g_IsInitialized = false;
            LOG_ERROR("Nsight Aftermath failed to initialize device ({:X})", static_cast<uint32_t>(result));
            return;
        }

        LOG_INFO("Nsight Aftermath initialized");
    }

    static __forceinline bool IsInitializedAndFeatureEnabled(uint32_t featureFlag)
    {
        return g_IsInitialized && (g_FeatureFlags & featureFlag) == featureFlag;
    }

    static void HandleAftermathError(GFSDK_Aftermath_Result result)
    {
        std::string message;

        switch (result)
        {
        case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
            message = "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
            break;
        case GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported:
            message = "Nsight Aftermath is incompatible with D3D API interception, such as PIX or Nsight Graphics.";
            break;
        default:
            message = StringUtils::Format("Nsight Aftermath Error: 0x{:X}.", static_cast<uint32_t>(result));
            break;
        }

        GetApp()->CrashWithMessage("Nsight Aftermath Error", message, /* debugBreak */ true);
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
        path /= StringUtils::Format(L"Crash-{:%Y-%m-%d-%H-%M-%S}.nv-gpudmp", TimeUtils::GetLocalTime());
        return path.string();
    }

    static void GFSDK_AFTERMATH_CALL GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
    {
        const std::string path = GetCrashDumpFilePath();

        if (path.empty())
        {
            LOG_ERROR("Failed to create crash dump file");
            return;
        }

        std::ofstream fs(path, std::ios::out | std::ios::binary);
        fs.write(static_cast<const char*>(pGpuCrashDump), gpuCrashDumpSize);
    }

    static std::string GetShaderDebugInfoFilePath(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier)
    {
        fs::path path = StringUtils::Utf8ToUtf16(GetApp()->GetDataPath() + "/Logs/ShaderDebugInfo");

        if (!fs::exists(path) && !fs::create_directories(path))
        {
            LOG_ERROR("Failed to create directory: {}", path.string());
            return "";
        }

        // uint64_t 转大写 16 进制，并保留前导 0
        path /= StringUtils::Format(L"{:016X}-{:016X}.nvdbg", identifier.id[0], identifier.id[1]);
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

        const std::string path = GetShaderDebugInfoFilePath(identifier);

        if (path.empty())
        {
            LOG_ERROR("Failed to create shader debug info file");
            return;
        }

        // Write to file for later in-depth analysis of crash dumps with Nsight Graphics
        std::ofstream fs(path, std::ios::out | std::ios::binary);
        fs.write(static_cast<const char*>(pShaderDebugInfo), shaderDebugInfoSize);
    }

    static void GFSDK_AFTERMATH_CALL CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addValue, void* pUserData)
    {
        addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, GetApp()->GetProjectName().c_str());
        addValue(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "1.0.0"); // TODO app version
    }

    static void GFSDK_AFTERMATH_CALL ResolveMarkerCallback(const void* pMarkerData, const uint32_t markerDataSize, void* pUserData, void** ppResolvedMarkerData, uint32_t* pResolvedMarkerDataSize)
    {
    }

    bool NsightAftermath::HandleGpuCrash()
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
        if (!IsInitializedAndFeatureEnabled(GFSDK_Aftermath_FeatureFlags_EnableResourceTracking))
        {
            return nullptr;
        }

        GFSDK_Aftermath_ResourceHandle handle = {};
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_RegisterResource(resource, &handle));
        return reinterpret_cast<void*>(handle);
    }

    void NsightAftermath::UnregisterResource(void* resourceHandle)
    {
        if (!IsInitializedAndFeatureEnabled(GFSDK_Aftermath_FeatureFlags_EnableResourceTracking))
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
        if (!IsInitializedAndFeatureEnabled(GFSDK_Aftermath_FeatureFlags_EnableMarkers))
        {
            return nullptr;
        }

        GFSDK_Aftermath_ContextHandle handle = {};
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DX12_CreateContextHandle(cmdList, &handle));
        return reinterpret_cast<void*>(handle);
    }

    void NsightAftermath::ReleaseContextHandle(void* contextHandle)
    {
        if (!IsInitializedAndFeatureEnabled(GFSDK_Aftermath_FeatureFlags_EnableMarkers))
        {
            return;
        }

        auto handle = reinterpret_cast<GFSDK_Aftermath_ContextHandle>(contextHandle);
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_ReleaseContextHandle(handle));
    }

    void NsightAftermath::SetEventMarker(void* contextHandle, const std::string& label)
    {
        if (!IsInitializedAndFeatureEnabled(GFSDK_Aftermath_FeatureFlags_EnableMarkers))
        {
            return;
        }

        auto handle = reinterpret_cast<GFSDK_Aftermath_ContextHandle>(contextHandle);
        uint32_t dataSize = static_cast<uint32_t>(label.size() + 1); // 需要加上 '\0' 字符
        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_SetEventMarker(handle, label.c_str(), dataSize));
    }
}
