#pragma once

#include <d3dx12.h>
#include <string>

namespace march
{
    struct NsightAftermath
    {
        static void InitializeBeforeDeviceCreation(bool fullFeatures);
        static void InitializeDevice(ID3D12Device* device);

        static bool HandleGpuCrash();

        static void* RegisterResource(ID3D12Resource* resource);
        static void UnregisterResource(void* resourceHandle);

        static void* CreateContextHandle(ID3D12CommandList* cmdList);
        static void ReleaseContextHandle(void* contextHandle);
        static void SetEventMarker(void* contextHandle, const std::string& label);
    };
}
