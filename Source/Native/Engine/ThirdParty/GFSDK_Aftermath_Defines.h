/*
 * Copyright (c) 2016-2024, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

/*
 *   █████  █████ ██████ ████  ████   ███████   ████  ██████ ██   ██
 *   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
 *   ██  ██ ██      ██   ██    ██  ██ ██ ██ ██ ██  ██   ██   ██   ██
 *   ██████ ████    ██   ████  █████  ██ ██ ██ ██████   ██   ███████
 *   ██  ██ ██      ██   ██    ██  ██ ██    ██ ██  ██   ██   ██   ██
 *   ██  ██ ██      ██   █████ ██  ██ ██    ██ ██  ██   ██   ██   ██   DEBUGGER
 *                                                           ██   ██
 *  ████████████████████████████████████████████████████████ ██ █ ██ ████████████
 */

#ifndef GFSDK_Aftermath_Defines_H
#define GFSDK_Aftermath_Defines_H

#if defined(_MSC_VER)
#if defined(_M_X86)
#define GFSDK_AFTERMATH_CALL __cdecl
#else
#define GFSDK_AFTERMATH_CALL
#endif
#elif defined(__clang__) || defined(__GNUC__)
#if defined(__i386__)
#define GFSDK_AFTERMATH_CALL __attribute__((cdecl))
#else
#define GFSDK_AFTERMATH_CALL
#endif
#else
#error "Unsupported compiler"
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdbool.h>
#include <stdint.h>
#endif

// Library stuff...
#define GFSDK_Aftermath_PFN typedef GFSDK_Aftermath_Result

#if defined(_MSC_VER)
#ifdef EXPORTS
#define GFSDK_Aftermath_DLLSPEC __declspec(dllexport)
#else
#define GFSDK_Aftermath_DLLSPEC
#endif
#elif defined(__clang__) || defined(__GNUC__)
#define GFSDK_Aftermath_DLLSPEC __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
#define GFSDK_Aftermath_API extern "C" GFSDK_Aftermath_DLLSPEC GFSDK_Aftermath_Result GFSDK_AFTERMATH_CALL
#else
#define GFSDK_Aftermath_API GFSDK_Aftermath_DLLSPEC GFSDK_Aftermath_Result GFSDK_AFTERMATH_CALL
#endif

#pragma pack(push, 8)

/////////////////////////////////////////////////////////////////////////
//
// Helper macros for declaring struct members and types with guaranteed properties.
//
/////////////////////////////////////////////////////////////////////////
#define GFSDK_AFTERMATH_DECLARE_HANDLE(name) \
    struct name##__                          \
    {                                        \
        int32_t ID;                          \
    };                                       \
    typedef struct name##__* name
#ifdef __cplusplus
#define GFSDK_AFTERMATH_DECLARE_ENUM(name) enum GFSDK_Aftermath_##name : uint32_t
#else
#define GFSDK_AFTERMATH_DECLARE_ENUM(name)   \
    typedef uint32_t GFSDK_Aftermath_##name; \
    enum GFSDK_Aftermath_##name
#endif
#define GFSDK_AFTERMATH_DECLARE_POINTER_MEMBER(type, name) \
    union                                                  \
    {                                                      \
        type name;                                         \
        uint64_t ptr_align_##name;                         \
    }
#define GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(name) \
    union                                            \
    {                                                \
        bool name;                                   \
        uint32_t bool_align_##name;                  \
    }

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Version
// ---------------------------------
//
// Aftermath API version
//
// NOTE: The Aftermath SDK does not guarantee compatibility between different
// API versions. Therefore, the API version of the header files used for
// building the application needs to match the API version of the Aftermath
// library loaded by the application.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Version){
    GFSDK_Aftermath_Version_API = 0x0000218 // Version 2.24
};

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Result
// ---------------------------------
//
// Result codes returned by Aftermath API functions
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Result){
    // The call was successful.
    GFSDK_Aftermath_Result_Success = 0x1,

    // The requested functionality is not available.
    GFSDK_Aftermath_Result_NotAvailable = 0x2,

    // The call failed with an unspecified failure.
    GFSDK_Aftermath_Result_Fail = 0xBAD00000,

    // The callee tries to use a library version which does not match the built
    // binary.
    GFSDK_Aftermath_Result_FAIL_VersionMismatch = GFSDK_Aftermath_Result_Fail | 1,

    // The library hasn't been initialized, see 'GFSDK_Aftermath_DX*_Initialize'.
    GFSDK_Aftermath_Result_FAIL_NotInitialized = GFSDK_Aftermath_Result_Fail | 2,

    // The callee tries to use the library with a non-supported GPU. Only NVIDIA GPUs
    // are supported.
    GFSDK_Aftermath_Result_FAIL_InvalidAdapter = GFSDK_Aftermath_Result_Fail | 3,

    // The callee passed an invalid parameter to the library, likely a null pointer
    // or a bad handle.
    GFSDK_Aftermath_Result_FAIL_InvalidParameter = GFSDK_Aftermath_Result_Fail | 4,

    // Something weird happened that caused the library to fail for an unknown
    // reason.
    GFSDK_Aftermath_Result_FAIL_Unknown = GFSDK_Aftermath_Result_Fail | 5,

    // Got a failure from the graphics API.
    GFSDK_Aftermath_Result_FAIL_ApiError = GFSDK_Aftermath_Result_Fail | 6,

    // Make sure that the NvAPI DLL is up to date.
    GFSDK_Aftermath_Result_FAIL_NvApiIncompatible = GFSDK_Aftermath_Result_Fail | 7,

    // It would appear as though a call has been made to fetch the Aftermath data for
    // a context that hasn't yet been used with the event marker API.
    GFSDK_Aftermath_Result_FAIL_GettingContextDataWithNewCommandList = GFSDK_Aftermath_Result_Fail | 8,

    // Looks like the library has already been initialized.
    GFSDK_Aftermath_Result_FAIL_AlreadyInitialized = GFSDK_Aftermath_Result_Fail | 9,

    // A debug layer not compatible with Aftermath has been detected.
    GFSDK_Aftermath_Result_FAIL_D3DDebugLayerNotCompatible = GFSDK_Aftermath_Result_Fail | 10,

    // Aftermath failed to initialize in the graphics driver.
    GFSDK_Aftermath_Result_FAIL_DriverInitFailed = GFSDK_Aftermath_Result_Fail | 11,

    // Aftermath v2.x requires NVIDIA graphics driver version 387.xx or beyond.
    GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported = GFSDK_Aftermath_Result_Fail | 12,

    // The system ran out of memory for allocations.
    GFSDK_Aftermath_Result_FAIL_OutOfMemory = GFSDK_Aftermath_Result_Fail | 13,

    // No need to get data on bundles, as markers execute on the command list.
    GFSDK_Aftermath_Result_FAIL_GetDataOnBundle = GFSDK_Aftermath_Result_Fail | 14,

    // No need to get data on deferred contexts, as markers execute on the immediate
    // context.
    GFSDK_Aftermath_Result_FAIL_GetDataOnDeferredContext = GFSDK_Aftermath_Result_Fail | 15,

    // This feature hasn't been enabled at initialization - see 'GFSDK_Aftermath_FeatureFlags'.
    GFSDK_Aftermath_Result_FAIL_FeatureNotEnabled = GFSDK_Aftermath_Result_Fail | 16,

    // No resources have ever been registered.
    GFSDK_Aftermath_Result_FAIL_NoResourcesRegistered = GFSDK_Aftermath_Result_Fail | 17,

    // This resource has never been registered.
    GFSDK_Aftermath_Result_FAIL_ThisResourceNeverRegistered = GFSDK_Aftermath_Result_Fail | 18,

    // The functionality is not supported for UWP applications.
    GFSDK_Aftermath_Result_FAIL_NotSupportedInUWP = GFSDK_Aftermath_Result_Fail | 19,

    // The version of the D3D DLL is not compatible with Aftermath.
    GFSDK_Aftermath_Result_FAIL_D3dDllNotSupported = GFSDK_Aftermath_Result_Fail | 20,

    // D3D DLL interception is not compatible with Aftermath.
    GFSDK_Aftermath_Result_FAIL_D3dDllInterceptionNotSupported = GFSDK_Aftermath_Result_Fail | 21,

    // Aftermath is disabled on the system by the current user.
    // On Windows, this is controlled by a Windows registry key:
    //    KeyPath   : HKEY_CURRENT_USER\Software\NVIDIA Corporation\Nsight Aftermath
    //    KeyValue  : ForceOff
    //    ValueType : REG_DWORD
    //    ValueData : Any value != 0 will force the functionality of the Aftermath
    //                SDK off on the system.
    //
    // On Linux, this is controlled by an environment variable:
    //    Name      : NV_AFTERMATH_FORCE_OFF
    //    Value     : Any value != "0" will force the functionality of the Aftermath SDK
    //                off.
    GFSDK_Aftermath_Result_FAIL_Disabled = GFSDK_Aftermath_Result_Fail | 22,

    // Markers cannot be set on queue or device contexts.
    GFSDK_Aftermath_Result_FAIL_NotSupportedOnContext = GFSDK_Aftermath_Result_Fail | 23,
};

/////////////////////////////////////////////////////////////////////////
//
// Macro for simplified checking of API call result code.
//
/////////////////////////////////////////////////////////////////////////
#define GFSDK_Aftermath_SUCCEED(value) (((value) & 0xFFF00000) != GFSDK_Aftermath_Result_Fail)

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Context_Status
// ---------------------------------
//
// Status of an Aftermath context.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Context_Status){
    // The GPU has not started processing this command list yet.
    GFSDK_Aftermath_Context_Status_NotStarted = 0,

    // This command list has begun execution on the GPU.
    GFSDK_Aftermath_Context_Status_Executing,

    // This command list has finished execution on the GPU.
    GFSDK_Aftermath_Context_Status_Finished,

    // This context has an invalid state, which could be caused by an error.
    GFSDK_Aftermath_Context_Status_Invalid,
};

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Device_Status
// ---------------------------------
//
// Status of a D3D device.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Device_Status){
    // The device is still active and hasn't gone down.
    GFSDK_Aftermath_Device_Status_Active = 0,

    // A long running shader/operation has caused a GPU timeout. Reconfiguring the
    // timeout length might help tease out the problem.
    GFSDK_Aftermath_Device_Status_Timeout,

    // Run out of memory to complete operations.
    GFSDK_Aftermath_Device_Status_OutOfMemory,

    // An invalid VA access has caused a fault.
    GFSDK_Aftermath_Device_Status_PageFault,

    // The GPU has stopped executing.
    GFSDK_Aftermath_Device_Status_Stopped,

    // The device has been reset.
    GFSDK_Aftermath_Device_Status_Reset,

    // Unknown problem - likely using an older driver incompatible with this
    // Aftermath feature.
    GFSDK_Aftermath_Device_Status_Unknown,

    // An invalid rendering call has percolated through the driver.
    GFSDK_Aftermath_Device_Status_DmaFault,

    // The device was removed but no GPU fault was detected.
    GFSDK_Aftermath_Device_Status_DeviceRemovedNoGpuFault,
};

#pragma pack(pop)

#endif // GFSDK_Aftermath_Defines_H
