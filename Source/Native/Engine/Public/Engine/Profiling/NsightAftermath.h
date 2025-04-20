#pragma once

#include <directx/d3dx12.h>
#include <string>

namespace march
{
    struct NsightAftermath
    {
        enum class FeatureFlags
        {
            Minimum = 0,
            EnableMarkers = 1 << 0,
            EnableResourceTracking = 1 << 1,
            CallStackCapturing = 1 << 2,
            GenerateShaderDebugInfo = 1 << 3,
            EnableShaderErrorReporting = 1 << 4,
            All = EnableMarkers | EnableResourceTracking | CallStackCapturing | GenerateShaderDebugInfo | EnableShaderErrorReporting,
        };

        static void InitializeBeforeDeviceCreation();
        static void InitializeDevice(ID3D12Device* device, FeatureFlags features);
        static bool OnGpuCrash();

        static void* RegisterResource(ID3D12Resource* resource);
        static void UnregisterResource(void* resourceHandle);

        static void* CreateContextHandle(ID3D12CommandList* cmdList);
        static void ReleaseContextHandle(void* contextHandle);
        static void SetEventMarker(void* contextHandle, const std::string& label);
    };

    DEFINE_ENUM_FLAG_OPERATORS(NsightAftermath::FeatureFlags);
}
