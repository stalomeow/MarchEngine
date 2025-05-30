/*
 * Copyright (c) 2019-2024, NVIDIA CORPORATION.  All rights reserved.
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
 *
 *
 *  HOW TO DECODE AFTERMATH GPU CRASH DUMPS:
 *  ----------------------------------------
 *
 *  1)  Call 'GFSDK_Aftermath_GpuCrashDump_CreateDecoder', to create a decoder object for
 *      a GPU crash dump.
 *
 *
 *  2) Call one or more of the 'GFSDK_Aftermath_GpuCrashDump_Get*' functions with this
 *     decoder, to query information from the GPU crash dump.
 *
 *     Some of the functions require caller allocated buffers to return the data. Those
 *     are accompanied with a corresponding 'GFSDK_Aftermath_GpuCrashDump_Get*Count'
 *     function to query the element count the caller has to reserve for these buffers.
 *
 *     If the requested data is not available in the GPU crash dump, the functions
 *     will return with 'GFSDK_Aftermath_Result_NotAvailable'.
 *
 *
 *  3) Call 'GFSDK_Aftermath_GpuCrashDump_DestroyDecoder', to destroy the decoder object
 *     and cleanup all related memory.
 *
 */

#ifndef GFSDK_Aftermath_CrashDumpDecoding_H
#define GFSDK_Aftermath_CrashDumpDecoding_H

#include "GFSDK_Aftermath_Defines.h"

#pragma pack(push, 8)

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////
//
// Constants used in GPU crash dump decoding functions.
//
/////////////////////////////////////////////////////////////////////////
enum
{
    GFSDK_Aftermath_MAX_STRING_LENGTH = 127,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderDebugInfoIdentifier
// ---------------------------------
//
// Unique identifier for shader debug information.
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_ShaderDebugInfoIdentifier
{
    uint64_t id[2];
} GFSDK_Aftermath_ShaderDebugInfoIdentifier;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderBinaryHash
// ---------------------------------
//
// Unique identifier for shader binaries.
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_ShaderBinaryHash
{
    uint64_t hash;
} GFSDK_Aftermath_ShaderBinaryHash;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderDebugName
// ---------------------------------
//
// Shader DebugName, i.e., a unique identifier for shader source debug information.
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_ShaderDebugName
{
    char name[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];
} GFSDK_Aftermath_ShaderDebugName;

#if defined(VULKAN_H_)
////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_SpirvCode
// ---------------------------------
//
// SPIR-V shader code.
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_SpirvCode
{
    GFSDK_AFTERMATH_DECLARE_POINTER_MEMBER(const void*, pData);
    uint32_t size;
} GFSDK_Aftermath_SpirvCode;
#endif

////////////////////////////////////////////////////////////////////////
//  GFSDK_Aftermath_GraphicsApi
// ---------------------------------
//
// Graphics API
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(GraphicsApi){
    GFSDK_Aftermath_GraphicsApi_Unknown = 0,
    GFSDK_Aftermath_GraphicsApi_D3D_10_0 = 1,
    GFSDK_Aftermath_GraphicsApi_D3D_10_1 = 2,
    GFSDK_Aftermath_GraphicsApi_D3D_11_0 = 3,
    GFSDK_Aftermath_GraphicsApi_D3D_11_1 = 4,
    GFSDK_Aftermath_GraphicsApi_D3D_11_2 = 5,
    GFSDK_Aftermath_GraphicsApi_D3D_12_0 = 6,
    GFSDK_Aftermath_GraphicsApi_Vulkan = 7,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_BaseInfo
// ---------------------------------
//
// GPU crash dump - base information
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_BaseInfo
{
    char applicationName[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];
    char creationDate[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];
    uint32_t creationTickCount;
    uint32_t pid;
    GFSDK_Aftermath_GraphicsApi graphicsApi;
} GFSDK_Aftermath_GpuCrashDump_BaseInfo;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_DeviceInfo
// ---------------------------------
//
// GPU crash dump - device information
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_DeviceInfo
{
    GFSDK_Aftermath_Device_Status status;
    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(adapterReset);
    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(engineReset);
} GFSDK_Aftermath_GpuCrashDump_DeviceInfo;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_SystemInfo
// ---------------------------------
//
// GPU crash dump - system information
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_SystemInfo
{
    char osVersion[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];
    struct DisplayDriverVersion
    {
        uint32_t major;
        uint32_t minor;
    } displayDriver;
} GFSDK_Aftermath_GpuCrashDump_SystemInfo;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GpuInfo
// ---------------------------------
//
// GPU crash dump - GPU information
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_GpuInfo
{
    char adapterName[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];
    char generationName[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];
    uint64_t adapterLUID;
} GFSDK_Aftermath_GpuCrashDump_GpuInfo;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_FaultType
// ---------------------------------
//
// GPU crash dump - page fault type
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(FaultType){
    GFSDK_Aftermath_FaultType_Unknown = 0,
    GFSDK_Aftermath_FaultType_AddressTranslationError,
    GFSDK_Aftermath_FaultType_IllegalAccessError,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_AccessType
// ---------------------------------
//
// GPU crash dump - page fault access type
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(AccessType){
    GFSDK_Aftermath_AccessType_Unknown = 0,
    GFSDK_Aftermath_AccessType_Read,
    GFSDK_Aftermath_AccessType_Write,
    GFSDK_Aftermath_AccessType_Atomic,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Engine
// ---------------------------------
//
// GPU crash dump - page fault GPU engine
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Engine){
    GFSDK_Aftermath_Engine_Unknown = 0,
    GFSDK_Aftermath_Engine_Graphics,
    GFSDK_Aftermath_Engine_GraphicsCompute,
    GFSDK_Aftermath_Engine_Display,
    GFSDK_Aftermath_Engine_CopyEngine,
    GFSDK_Aftermath_Engine_VideoDecoder,
    GFSDK_Aftermath_Engine_VideoEncoder,
    GFSDK_Aftermath_Engine_Other,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Client
// ---------------------------------
//
// GPU crash dump - page fault GPU client
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Client){
    GFSDK_Aftermath_Client_Unknown = 0,
    GFSDK_Aftermath_Client_HostInterface,
    GFSDK_Aftermath_Client_FrontEnd,
    GFSDK_Aftermath_Client_PrimitiveDistributor,
    GFSDK_Aftermath_Client_GraphicsProcessingCluster,
    GFSDK_Aftermath_Client_PolymorphEngine,
    GFSDK_Aftermath_Client_RasterEngine,
    GFSDK_Aftermath_Client_Rasterizer2D,
    GFSDK_Aftermath_Client_RenderOutputUnit,
    GFSDK_Aftermath_Client_TextureProcessingCluster,
    GFSDK_Aftermath_Client_CopyEngine,
    GFSDK_Aftermath_Client_VideoDecoder,
    GFSDK_Aftermath_Client_VideoEncoder,
    GFSDK_Aftermath_Client_Other,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ResourceResidency
// ---------------------------------
//
// GPU crash dump - page fault resource residency
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(ResourceResidency){
    GFSDK_Aftermath_ResourceResidency_Unknown = 0,
    GFSDK_Aftermath_ResourceResidency_FullyResident,
    GFSDK_Aftermath_ResourceResidency_Evicted,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_PageFaultInfo
// ---------------------------------
//
// GPU crash dump - page fault information
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_PageFaultInfo
{
    uint64_t faultingGpuVA;
    GFSDK_Aftermath_FaultType faultType;
    GFSDK_Aftermath_AccessType accessType;
    GFSDK_Aftermath_Engine engine;
    GFSDK_Aftermath_Client client;
    uint32_t resourceInfoCount;
} GFSDK_Aftermath_GpuCrashDump_PageFaultInfo;

typedef struct GFSDK_Aftermath_GpuCrashDump_ResourceInfo
{
    uint64_t gpuVa;
    uint64_t size;

    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mipLevels;

    uint32_t format; // DXGI_Format for DX, VkFormat for Vulkan

    // For DX12, and only if the application registers the resource pointers using
    //     'GFSDK_Aftermath_DX12_RegisterResource', this is the 'ID3D12Resource' pointer
    //     that was registered and cast to 'uint64_t'.
    //     Note, if the resource was destroyed, this is a dangling pointer.
    // For DX11, this is unsupported and will always be 0.
    // For Vulkan, this is the VkImage or VkBuffer handle cast to 'uint64_t'.
    //     Note, if the resource was destroyed, this is a stale handle.
    uint64_t apiResource;

    char debugName[GFSDK_Aftermath_MAX_STRING_LENGTH + 1];

    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(bIsBufferHeap);
    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(bIsStaticTextureHeap);
    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(bIsRenderTargetOrDepthStencilViewHeap);
    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(bPlacedResource);

    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(bWasDestroyed);
    GFSDK_Aftermath_ResourceResidency residency;
    uint32_t createDestroyTickCount;
} GFSDK_Aftermath_GpuCrashDump_ResourceInfo;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderType
// ---------------------------------
//
// GPU crash dump - shader types
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(ShaderType){
    GFSDK_Aftermath_ShaderType_Unknown = 0,
    GFSDK_Aftermath_ShaderType_Vertex,
    GFSDK_Aftermath_ShaderType_Tessellation_Control,
    GFSDK_Aftermath_ShaderType_Hull = GFSDK_Aftermath_ShaderType_Tessellation_Control,
    GFSDK_Aftermath_ShaderType_Tessellation_Evaluation,
    GFSDK_Aftermath_ShaderType_Domain = GFSDK_Aftermath_ShaderType_Tessellation_Evaluation,
    GFSDK_Aftermath_ShaderType_Geometry,
    GFSDK_Aftermath_ShaderType_Fragment,
    GFSDK_Aftermath_ShaderType_Pixel = GFSDK_Aftermath_ShaderType_Fragment,
    GFSDK_Aftermath_ShaderType_Compute,
    GFSDK_Aftermath_ShaderType_RayTracing_RayGeneration,
    GFSDK_Aftermath_ShaderType_RayTracing_Miss,
    GFSDK_Aftermath_ShaderType_RayTracing_Intersection,
    GFSDK_Aftermath_ShaderType_RayTracing_AnyHit,
    GFSDK_Aftermath_ShaderType_RayTracing_ClosestHit,
    GFSDK_Aftermath_ShaderType_RayTracing_Callable,
    GFSDK_Aftermath_ShaderType_RayTracing_Internal,
    GFSDK_Aftermath_ShaderType_Mesh,
    GFSDK_Aftermath_ShaderType_Task,
};

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_ShaderInfo
// ---------------------------------
//
// GPU crash dump - shader information
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_ShaderInfo
{
    // NOTE: This shader hash value is not necessarily the same as the GFSDK_Aftermath_ShaderBinaryHash
    // value for this shader info, which must be calculated with GFSDK_Aftermath_GetShaderHashForShaderInfo.
    uint64_t shaderHash;
    uint64_t shaderInstance;
    GFSDK_AFTERMATH_DECLARE_BOOLEAN_MEMBER(isInternal);
    GFSDK_Aftermath_ShaderType shaderType;
} GFSDK_Aftermath_GpuCrashDump_ShaderInfo;

////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_Context_Type
// ---------------------------------
//
// GPU crash dump - event marker context type
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(Context_Type){
    GFSDK_Aftermath_Context_Type_Invalid = 0,
    GFSDK_Aftermath_Context_Type_Immediate,
    GFSDK_Aftermath_Context_Type_CommandList,
    GFSDK_Aftermath_Context_Type_Bundle,
    GFSDK_Aftermath_Context_Type_CommandQueue};

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_EventMarkerDataOwnership
// ---------------------------------
//
// GPU crash dump - Event marker data ownership.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(EventMarkerDataOwnership){
    // Data is owned by the user application.
    GFSDK_Aftermath_EventMarkerDataOwnership_User = 0,

    // Data is part of the GPU crash dump and is owned by the decoder.
    GFSDK_Aftermath_EventMarkerDataOwnership_Decoder,
};

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo
// ---------------------------------
//
// GPU crash dump - Aftermath event marker information.
//
// NOTE: If 'GFSDK_Aftermath_SetEventMarker' was called with 'markerDataSize=0',
// 'markerDataOwnership' will be set to 'GFSDK_Aftermath_EventMarkerDataOwnership_User'
// and the 'markerData' pointer will be only valid within the context of the process
// setting the marker and if the application properly manages the lifetime of the
// pointed to data. It is the responsibility of the caller to ensure that the pointer
// is valid before accessing the pointed to data.
//
/////////////////////////////////////////////////////////////////////////
typedef struct GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo
{
    uint64_t contextId;
    GFSDK_Aftermath_Context_Status contextStatus;
    GFSDK_Aftermath_Context_Type contextType;
    GFSDK_AFTERMATH_DECLARE_POINTER_MEMBER(const void*, markerData);
    GFSDK_Aftermath_EventMarkerDataOwnership markerDataOwnership;
    uint32_t markerDataSize;
} GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo;

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDumpDecoderFlags
// ---------------------------------
//
// Flags that allow to specify the data to be included in the JSON generated by
// 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON'.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(GpuCrashDumpDecoderFlags){
    // Include basic information about the GPU crash dump.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_BASE_INFO = 0x1,

    // Include information about the device state.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_DEVICE_INFO = 0x2,

    // Include information about the OS.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_OS_INFO = 0x4,

    // Include information about the display driver.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_DISPLAY_DRIVER_INFO = 0x8,

    // Include information about the GPU.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_GPU_INFO = 0x10,

    // Include information about page faults (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_PAGE_FAULT_INFO = 0x20,

    // Include information about shaders (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_INFO = 0x40,

    // Include information about active warps (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_WARP_STATE_INFO = 0x80,

    // Try to map shader addresses to source or intermediate assembly lines using
    // additional information provided through 'shaderDebugInfoLookupCb',
    // 'shaderLookupCb', and 'shaderSourceDebugInfoLookupCb', if provided.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_MAPPING_INFO = 0x100,

    // Include Aftermath event marker data (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_EVENT_MARKER_INFO = 0x200,

    // Include automatic event marker call stack data (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_CALL_STACK_INFO = 0x400,

    // Include user provided GPU crash dump description values (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_DESCRIPTION_INFO = 0x800,

    // Include information about faulted warps (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_FAULTED_WARP_INFO = 0x1000,

    // Include information about the fingerprint of the GPU crash dump (if available).
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_FINGERPRINT_INFO = 0x2000,

    // Include all available information.
    GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO = 0x3FFF,
};

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDumpFormatterFlags
// ---------------------------------
//
// Flags controlling the formatting of the JSON generated by
// 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON'.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_ENUM(GpuCrashDumpFormatterFlags){
    // No special formatting
    GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE = 0x0,

    // Remove all unnecessary whitespace from formatted string.
    GFSDK_Aftermath_GpuCrashDumpFormatterFlags_CONDENSED_OUTPUT = 0x1,

    // Use UTF8 encoding.
    GFSDK_Aftermath_GpuCrashDumpFormatterFlags_UTF8_OUTPUT = 0x2,
};

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_Decoder
// ---------------------------------
//
// GPU crash dump decoder handle.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_AFTERMATH_DECLARE_HANDLE(GFSDK_Aftermath_GpuCrashDump_Decoder);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_SetData
// ---------------------------------
//
// Callback function for providing shader debug information and shader binary data to
// the GPU crash dump decoder.
//
/////////////////////////////////////////////////////////////////////////
typedef void(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_SetData)(const void* pData, uint32_t size);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderDebugInfoLookupCb
// ---------------------------------
//
// Callback used by the GPU crash dump decoder to query shader debug information. See
// the description of 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON' for more details.
//
/////////////////////////////////////////////////////////////////////////
typedef void(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_ShaderDebugInfoLookupCb)(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderLookupCb
// ---------------------------------
//
// Callback used by the GPU crash dump decoder to query shader binary information.
// See the description of 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON' for more
// details.
//
/////////////////////////////////////////////////////////////////////////
typedef void(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_ShaderLookupCb)(const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_ShaderSourceDebugInfoLookupCb
// ---------------------------------
//
// Callback used by the GPU crash dump decoder to query source shader debug
// information. See the description of 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON'
// for more details.
//
/////////////////////////////////////////////////////////////////////////
typedef void(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_ShaderSourceDebugInfoLookupCb)(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_CreateDecoder
// ---------------------------------
//
// apiVersion;
//      Must be set to 'GFSDK_Aftermath_Version_API'. Used for checking against
//      library version.
//
// pGpuCrashDump;
//      Pointer to GPU crash dump data captured in a 'GFSDK_Aftermath_GpuCrashDumpCb'
//      callback.
//
// gpuCrashDumpSize;
//      Size of GPU crash dump data in bytes.
//
// pDecoder;
//      Pointer to a decoder object owned by the caller that is initialized.
//
//// DESCRIPTION;
//      Create a decoder object that can be used to query information about the
//      provided GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
    GFSDK_Aftermath_Version apiVersion,
    const void* pGpuCrashDump,
    const uint32_t gpuCrashDumpSize,
    GFSDK_Aftermath_GpuCrashDump_Decoder* pDecoder);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_DestroyDecoder
// ---------------------------------
//
// decoder;
//      A valid GPU crash dump decoder object.
//
//// DESCRIPTION;
//      Free any data related to the passed in GPU crash dump decoder object.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetBaseInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pBaseInfo;
//      Pointer to data structure owned by the caller that is filled in with
//      information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query basic information from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    GFSDK_Aftermath_GpuCrashDump_BaseInfo* pBaseInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// key;
//      What value to query from the description section.
//
// pValueSize;
//      Populated with the size of the value in bytes (including 0-termination
//      of the string).
//
//// DESCRIPTION;
//      Query the size of a description value from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t key,
    uint32_t* pValueSize);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetDescription
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// key;
//      What value to query from the description section.
//
// valueBufferSize;
//      Size in bytes of the caller allocated results buffer 'pValue'.
//
// pValue;
//      Caller allocated results buffer.
//
//// DESCRIPTION;
//      Query description value from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetDescription(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t key,
    const uint32_t valueBufferSize,
    char* pValue);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pDeviceInfo;
//      Pointer to data structure owned by the caller that is filled in with
//      information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query device state information from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    GFSDK_Aftermath_GpuCrashDump_DeviceInfo* pDeviceInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetSystemInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
//  pSystemInfo;
//      Pointer to data structure owned by the caller that is filled in with
//      information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query system information (OS, display driver) from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetSystemInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    GFSDK_Aftermath_GpuCrashDump_SystemInfo* pSystemInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetGpuInfoCount
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pGpuCount;
//      Populated with the number of GPU entries from the GPU crash dump.
//
//// DESCRIPTION;
//      Query number of GPU entries from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetGpuInfoCount(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    uint32_t* pGpuCount);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetGpuInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// gpuInfoBufferCount;
//      Number of elements in caller allocated array passed in 'pGpuInfo'.
//
// pGpuInfo;
//      Pointer to caller allocated array of 'GFSDK_Aftermath_GpuCrashDump_GpuInfo'
//      that is filled in with information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query information about the GPUs from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetGpuInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t gpuInfoBufferCount,
    GFSDK_Aftermath_GpuCrashDump_GpuInfo* pGpuInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pPageFaultInfo;
//      Pointer to data structure owned by the caller that is filled in with
//      information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query page fault information from a GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    GFSDK_Aftermath_GpuCrashDump_PageFaultInfo* pPageFaultInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetPageFaultResourceInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// resourceInfoCount;
//      Number of elements in caller allocated array passed in 'pResourceInfo'.
//
// pResourceInfo;
//      Pointer to caller allocated array of 'GFSDK_Aftermath_GpuCrashDump_ResourceInfo'
//      that is filled in with information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query resource information about the page fault from the GPU crash dump.
//      The resource info count can be queried by calling 'GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo'.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetPageFaultResourceInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t resourceInfoCount,
    GFSDK_Aftermath_GpuCrashDump_ResourceInfo* pResourceInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pShaderCount;
//      Populated with the number of active shaders from the GPU crash dump.
//
//// DESCRIPTION;
//      Query the number of active shaders from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    uint32_t* pShaderCount);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// shaderInfoBufferCount;
//      Number of elements in caller allocated array passed in 'pShaderInfo'.
//
// pShaderInfo;
//      Pointer to caller allocated array of'GFSDK_Aftermath_GpuCrashDump_ShaderInfo'
//      that is filled in with information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query information about active shaders from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t shaderInfoBufferCount,
    GFSDK_Aftermath_GpuCrashDump_ShaderInfo* pShaderInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfoCount
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pMarkerCount;
//      Populated with the number of event markers from the GPU crash dump.
//
//// DESCRIPTION;
//      Query the number of DX event markers or Vulkan checkpoint markers from
//      the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfoCount(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    uint32_t* pMarkerCount);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// markerInfoBufferCount;
//      Number of elements in caller allocated array passed in 'pMarkerInfo'.
//
// pMarkerInfo;
//      Pointer to caller allocated array of
//      'GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo' that is filled in with
//      information from the GPU crash dump.
//
//// DESCRIPTION;
//      Query information about DX event markers or Vulkan checkpoint markers
//      from the GPU crash dump.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t markerInfoBufferCount,
    GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo* pMarkerInfo);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GenerateJSON
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// decoderFlags;
//      Flags that define what information to include in the decoding. Bitwise OR of
//      any of the flags defined in 'GFSDK_Aftermath_GpuCrashDumpDecoderFlags'.
//
// formatFlags;
//      Flags controlling the formatting. Bitwise OR of any of the flags defined in
//      'GFSDK_Aftermath_GpuCrashDumpFormatterFlags'.
//
// shaderDebugInfoLookupCb;
//      Callback used by the decoder to query shader debug information for mapping
//      shader addresses to source or intermediate assembly line.
//
//      Optional, can be NULL.
//
//      Used when 'GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_MAPPING_INFO' is
//      set in 'decoderFlags'.
//
// shaderLookupCb;
//      Callback used by the decoder to query shader information for mapping shader
//      addresses to shader intermediate assembly (DXIL/SPIR-V) or source.
//
//      Optional, can be NULL.
//
//      Used when 'GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_MAPPING_INFO' is
//      set in 'decoderFlags'.
//
// shaderSourceDebugInfoLookupCb;
//      Callback used by the decoder to query high-level shader debug information for
//      mapping shader addresses to shader source, if the shaders used by the
//      application are stripped of debug information. This lookup is done by the
//      shader's DebugName, a unique identifier of the source debug information.
//
//      Optional, can be NULL.
//
//      Used when 'GFSDK_Aftermath_GpuCrashDumpDecoderFlags_SHADER_MAPPING_INFO' is
//      set in 'decoderFlags'.
//
//      For DXIL shaders DebugName is generated by the dxc compiler and is defined here:
//      https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SourceLevelDebuggingHLSL.rst#using-debug-names.
//
//      The following variants of generating source shader debug information for DXIL shaders
//      are supported:
//
//      1) Compile and use a full shader blob
//
//         Compile the shaders with the debug information. Use the full (i.e.,
//         not stripped) shader binary when running the application and make it
//         accessible through shaderLookupCb. In this case there is no need to
//         provide 'shaderSourceDebugInfoLookupCb'.
//
//         Compilation example:
//
//              dxc -Zi [..] -Fo shader.bin shader.hlsl
//
//      2) Compile and strip
//
//         Compile the shaders with debug information and then strip off the debug
//         information. Use the stripped shader binary data when running the
//         application.  Make the stripped shader binary data accessible through
//         shaderLookupCb.  In addition, make the non-stripped shader binary data
//         accessible through 'shaderSourceDebugInfoLookupCb'.
//
//         Compilation example:
//
//              dxc -Zi [..] -Fo full_shader.bin shader.hlsl
//              dxc -dumpbin -Qstrip_debug -Fo shader.bin full_shader.bin
//
//         The shader's DebugName required for implementing the
//         'shaderSourceDebugInfoLookupCb' may be extracted from the stripped or the
//         non-stripped shader binary data with 'GFSDK_Aftermath_GetShaderDebugName'.
//
//      3) Compile with separate debug information (and auto-generated debug data file name)
//
//         Compile the shaders with debug information and instruct the compiler to store
//         the debug meta data in a separate shader debug information file. The name of
//         the file generated by the compiler will match the DebugName of the shader.
//         Make the shader binary data accessible through 'shaderLookupCb'. In addition, make
//         the data from the compiler generated shader debug data file accessible through
//         'shaderSourceDebugInfoLookupCb'.
//
//         Compilation example:
//
//              dxc -Zi [..] -Fo shader.bin -Fd debugInfo\ shader.hlsl
//
//         The debug data file generated by the compiler does not contain any reference to
//         the shader's DebugName. It is the responsibility of the user providing the
//         'shaderSourceDebugInfoLookupCb' callback to implement a solution to lookup the
//         debug data based on the name of the generated debug data file.
//
//      4) Compile with separate debug information (and user-defined debug data file name)
//
//         Compile the shaders with debug information and instruct the compiler to
//         store the debug meta data in a separate shader debug information file. The
//         name of the file is freely chosen by the user. Make the shader binary
//         data accessible through 'shaderLookupCb'. In addition, make the data from
//         the compiler generated shader debug data file accessible through
//         'shaderSourceDebugInfoLookupCb'.
//
//         Compilation example:
//
//              dxc -Zi [..] -Fo shader.bin -Fd debugInfo\shader.dbg shader.hlsl
//
//         The debug data file generated by the compiler does not contain any
//         reference to the shader's DebugName. It is the responsibility of the
//         user providing the 'shaderSourceDebugInfoLookupCb' callback to
//         implement a solution that performs the lookup of the debug data
//         based on a mapping between the shader's DebugName and the debug data
//         file's name that was chosen for the compilation. The shader's
//         DebugName may be extracted from the shader binary data with
//         'GFSDK_Aftermath_GetShaderDebugName'.
//
//      For SPIR-V shaders the Aftermath SDK provides support for the following variants of
//      generating source shader debug information:
//
//      1) Compile and use a full shader blob
//
//         Compile the shaders with debug information. Use the full (i.e., not
//         stripped) shader binary when running the application and make it
//         accessible through 'shaderLookupCb'. In this case there is no need
//         to provide 'shaderSourceDebugInfoLookupCb'.
//
//         Compilation example using Vulkan SDK toolchain:
//
//              glslangValidator -V -g -o ./full/shader.spv shader.vert
//
//         Compilation example using the DirectX Shader Compiler:
//
//              dxc -spirv -Zi [..] -Fo shader.spv shader.hlsl
//
//      2) Compile and strip
//
//         Compile the shaders with debug information and then strip off the debug
//         information. Use the stripped shader binary data when running the
//         application. Make the stripped shader binary data accessible through
//         'shaderLookupCb'. In addition, make the non-stripped shader binary data
//         accessible through 'shaderSourceDebugInfoLookupCb'.
//
//         Compilation example using Vulkan SDK toolchain:
//
//              glslangValidator -V -g -o ./full/shader.spv shader.vert
//              spirv-remap --map all --strip-all --input full/shader.spv --output ./stripped/
//
//         Compilation example using the DirectX Shader Compiler:
//
//              dxc -spirv -Zi [..] -Fo ./full/shader.spv shader.hlsl
//              spirv-remap --map all --strip-all --input full/shader.spv --output ./stripped/
//
//         Then pass the content of ./full/shader.spv and ./stripped/shader.spv to
//         'GFSDK_Aftermath_GetDebugNameSpirv' to generate the debug name to use with
//         'shaderSourceDebugInfoLookupCb'.
//
// pUserData;
//      User data made available in callbacks.
//
// pJsonSize;
//      Populated with the size of the generated JSON data in bytes.
//
//// DESCRIPTION;
//      Decode the GPU crash dump to JSON format. The decoded JSON can be later
//      queried by calling 'GFSDK_Aftermath_GpuCrashDump_GetJSON'.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    uint32_t decoderFlags,
    uint32_t formatFlags,
    PFN_GFSDK_Aftermath_ShaderDebugInfoLookupCb shaderDebugInfoLookupCb,
    PFN_GFSDK_Aftermath_ShaderLookupCb shaderLookupCb,
    PFN_GFSDK_Aftermath_ShaderSourceDebugInfoLookupCb shaderSourceDebugInfoLookupCb,
    void* pUserData,
    uint32_t* pJsonSize);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GpuCrashDump_GetJSON
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// jsonBufferSize;
//      The size of the caller allocated buffer for the JSON data in bytes.
//
// pJson;
//      Caller allocated buffer populated with the JSON data (0-terminated string).
//
//// DESCRIPTION;
//      Copy the JSON generated by the last call to 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON'
//      into a caller provided buffer.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GpuCrashDump_GetJSON(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const uint32_t jsonBufferSize,
    char* pJson);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetShaderDebugInfoIdentifier
// ---------------------------------
//
// apiVersion;
//      Must be set to 'GFSDK_Aftermath_Version_API'. Used for checking against
//      library version.
//
// pShaderDebugInfo;
//      Pointer to shader debug information data captured in a
//      'GFSDK_Aftermath_ShaderDebugInfoCb' callback.
//
// shaderDebugInfoSize;
//      Size in bytes of the shader debug information data.
//
// pIdentifier;
//      Pointer to a 'GFSDK_Aftermath_ShaderDebugInfoIdentifier' structure that will
//      receive the result.
//
//// DESCRIPTION;
//      Read the shader debug information identifier from shader debug information.
//      The shader debug information identifier is required when implementing the
//      'GFSDK_Aftermath_ShaderDebugInfoLookupCb' callback.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
    GFSDK_Aftermath_Version apiVersion,
    const void* pShaderDebugInfo,
    const uint32_t shaderDebugInfoSize,
    GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier);

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetShaderHash
// ---------------------------------
//
// apiVersion;
//      Must be set to 'GFSDK_Aftermath_Version_API'. Used for checking against
//      library version.
//
// pShader;
//      The binary shader blob for which to compute the identifier.
//
// pShaderHash;
//      Pointer to a 'GFSDK_Aftermath_ShaderBinaryHash' structure that will receive
//      the computed shader hash.
//
//// DESCRIPTION;
//      Computes a shader hash uniquely identifying the provided DXBC shader binary.
//      This is, for example, required for comparison in the shader binary lookup by
//      'GFSDK_Aftermath_ShaderLookupCb' or for matching a
//      'GFSDK_Aftermath_GpuCrashDump_ShaderInfo' with a shader binary using
//      'GFSDK_Aftermath_GetShaderHashForShaderInfo'.
//
/////////////////////////////////////////////////////////////////////////
#if defined(__d3d12_h__)
GFSDK_Aftermath_API GFSDK_Aftermath_GetShaderHash(
    GFSDK_Aftermath_Version apiVersion,
    const D3D12_SHADER_BYTECODE* pShader,
    GFSDK_Aftermath_ShaderBinaryHash* pShaderHash);
#endif

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetShaderHashSpirv
// ---------------------------------
//
// apiVersion;
//      Must be set to 'GFSDK_Aftermath_Version_API'. Used for checking against
//      library version.
//
// pShader;
//      The SPIR-V shader binary for which to compute the identifier.
//
// pShaderHash;
//      Pointer to a 'GFSDK_Aftermath_ShaderBinaryHash' structure that will receive
//      the computed shader hash.
//
//// DESCRIPTION;
//      Computes a shader hash uniquely identifying the provided SPIR-V shader
//      binary. This is, for example, required for comparison in the shader binary
//      lookup by 'GFSDK_Aftermath_ShaderLookupCb' or for matching a
//      'GFSDK_Aftermath_GpuCrashDump_ShaderInfo' with a shader binary using
//      'GFSDK_Aftermath_GetShaderHashForShaderInfo'.
//
/////////////////////////////////////////////////////////////////////////
#if defined(VULKAN_H_)
GFSDK_Aftermath_API GFSDK_Aftermath_GetShaderHashSpirv(
    GFSDK_Aftermath_Version apiVersion,
    const GFSDK_Aftermath_SpirvCode* pShader,
    GFSDK_Aftermath_ShaderBinaryHash* pShaderHash);
#endif

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetShaderDebugName
// ---------------------------------
//
// apiVersion;
//      Must be set to 'GFSDK_Aftermath_Version_API'. Used for checking against
//      library version.
//
// pShader;
//      The binary shader data blob from which to extract the DebugName.
//
// pShaderDebugName;
//      Pointer to a 'GFSDK_Aftermath_ShaderDebugName' structure that will receive
//      the generated DebugName.
//
//// DESCRIPTION;
//      Extracts the shader's DebugName (if available) from the provided DXBC shader
//      binary. This is, for example, required for comparison in the shader debug
//      data lookup by 'GFSDK_Aftermath_ShaderSourceDebugInfoLookupCb'. For more
//      information about shader debug names please read:
//      https://github.com/microsoft/DirectXShaderCompiler/blob/master/docs/SourceLevelDebuggingHLSL.rst#using-debug-names.
//
/////////////////////////////////////////////////////////////////////////
#if defined(__d3d12_h__)
GFSDK_Aftermath_API GFSDK_Aftermath_GetShaderDebugName(
    GFSDK_Aftermath_Version apiVersion,
    const D3D12_SHADER_BYTECODE* pShader,
    GFSDK_Aftermath_ShaderDebugName* pShaderDebugName);
#endif

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetShaderDebugNameSpirv
// ---------------------------------
//
// apiVersion;
//      Must be set to 'GFSDK_Aftermath_Version_API'. Used for checking against
//      library version.
//
// pShader;
//      The not-stripped SPIR-V binary shader data of the shader pair for which to
//      generate the DebugName.
//
// pStrippedShader;
//      The stripped SPIR-V binary shader data of the shader pair for which to
//      generate the DebugName.
//
// pShaderDebugName;
//      Pointer to a 'GFSDK_Aftermath_ShaderDebugName' structure that will receive
//      the generated DebugName.
//
//// DESCRIPTION;
//      Generates a shader DebugName from the provided pair of SPIR-V shader binary
//      data. This is, for example, required for comparison in the shader debug data
//      lookup by 'GFSDK_Aftermath_ShaderSourceDebugInfoLookupCb'. For more
//      information about how to generate the pair of shader binaries, see the
//      description of 'GFSDK_Aftermath_GpuCrashDump_GenerateJSON'.
//
/////////////////////////////////////////////////////////////////////////
#if defined(VULKAN_H_)
GFSDK_Aftermath_API GFSDK_Aftermath_GetShaderDebugNameSpirv(
    GFSDK_Aftermath_Version apiVersion,
    const GFSDK_Aftermath_SpirvCode* pShader,
    const GFSDK_Aftermath_SpirvCode* pStrippedShader,
    GFSDK_Aftermath_ShaderDebugName* pShaderDebugName);
#endif

/////////////////////////////////////////////////////////////////////////
// GFSDK_Aftermath_GetShaderHashForShaderInfo
// ---------------------------------
//
// decoder;
//      A valid decoder object.
//
// pShaderInfo;
//      Pointer to 'GFSDK_Aftermath_GpuCrashDump_ShaderInfo' for which to generate
//      the 'GFSDK_Aftermath_ShaderBinaryHash'.
//
// pShaderHash;
//      Pointer to a 'GFSDK_Aftermath_ShaderBinaryHash' structure that will receive
//      the computed shader hash.
//
//// DESCRIPTION;
//      Computes a shader hash for the provided shader info. This is, for example,
//      useful for matching against the 'GFSDK_Aftermath_ShaderBinaryHash' values
//      calculated for shader binaries using 'GFSDK_Aftermath_GetShaderHash' or
//      'GFSDK_Aftermath_GetShaderHashSpirv'.
//
/////////////////////////////////////////////////////////////////////////
GFSDK_Aftermath_API GFSDK_Aftermath_GetShaderHashForShaderInfo(
    const GFSDK_Aftermath_GpuCrashDump_Decoder decoder,
    const GFSDK_Aftermath_GpuCrashDump_ShaderInfo* pShaderInfo,
    GFSDK_Aftermath_ShaderBinaryHash* pShaderHash);

/////////////////////////////////////////////////////////////////////////
//
// Function pointer definitions - if dynamic loading is preferred.
//
/////////////////////////////////////////////////////////////////////////

GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_CreateDecoder)(GFSDK_Aftermath_Version apiVersion, const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, GFSDK_Aftermath_GpuCrashDump_Decoder* pDecoder);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_DestroyDecoder)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetBaseInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, GFSDK_Aftermath_GpuCrashDump_BaseInfo* pBaseInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t key, uint32_t* pValueSize);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetDescription)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t key, const uint32_t valueBufferSize, char* pValue);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetDeviceInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, GFSDK_Aftermath_GpuCrashDump_DeviceInfo* pDeviceInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetSystemInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, GFSDK_Aftermath_GpuCrashDump_SystemInfo* pSystemInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetGpuInfoCount)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, uint32_t* pGpuCount);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetGpuInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t gpuInfoBufferCount, GFSDK_Aftermath_GpuCrashDump_GpuInfo* pGpuInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetPageFaultInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, GFSDK_Aftermath_GpuCrashDump_PageFaultInfo* pPageFaultInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetPageFaultResourceInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t resourceInfoCount, GFSDK_Aftermath_GpuCrashDump_ResourceInfo* pResourceInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfoCount)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, uint32_t* pShaderCount);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetActiveShadersInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t shaderInfoBufferCount, GFSDK_Aftermath_GpuCrashDump_ShaderInfo* pShaderInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfoCount)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t markerInfoBufferCount);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetEventMarkersInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t markerInfoBufferCount, GFSDK_Aftermath_GpuCrashDump_EventMarkerInfo* pMarkerInfo);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GenerateJSON)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, uint32_t decoderFlags, uint32_t formatFlags, PFN_GFSDK_Aftermath_ShaderDebugInfoLookupCb shaderDebugInfoLookupCb, PFN_GFSDK_Aftermath_ShaderLookupCb shaderLookupCb, PFN_GFSDK_Aftermath_ShaderSourceDebugInfoLookupCb shaderSourceDebugInfoLookupCb, void* pUserData, uint32_t* pJsonSize);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GpuCrashDump_GetJSON)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const uint32_t jsonBufferSize, char* pJson);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GetShaderDebugInfoIdentifier)(GFSDK_Aftermath_Version apiVersion, const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GetShaderHashForShaderInfo)(const GFSDK_Aftermath_GpuCrashDump_Decoder decoder, const GFSDK_Aftermath_GpuCrashDump_ShaderInfo* pShaderInfo, GFSDK_Aftermath_ShaderBinaryHash* pShaderHash);
#if defined(__d3d12_h__)
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GetShaderHash)(GFSDK_Aftermath_Version apiVersion, const D3D12_SHADER_BYTECODE* pShader, GFSDK_Aftermath_ShaderBinaryHash* pShaderHash);
GFSDK_Aftermath_PFN (*GPFN_GFSDK_Aftermath_GetShaderDebugName)(GFSDK_Aftermath_Version apiVersion, const D3D12_SHADER_BYTECODE* pShader, GFSDK_Aftermath_ShaderDebugName* pShaderDebugName);
#endif
#if defined(VULKAN_H_)
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GetShaderHashSpirv)(GFSDK_Aftermath_Version apiVersion, const GFSDK_Aftermath_SpirvCode* pShader, GFSDK_Aftermath_ShaderBinaryHash* pShaderHash);
GFSDK_Aftermath_PFN(GFSDK_AFTERMATH_CALL* PFN_GFSDK_Aftermath_GetShaderDebugNameSpirv)(GFSDK_Aftermath_Version apiVersion, const GFSDK_Aftermath_SpirvCode* pShader, const GFSDK_Aftermath_SpirvCode* pStrippedShader, GFSDK_Aftermath_ShaderDebugName* pShaderDebugName);
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#pragma pack(pop)

#endif // GFSDK_Aftermath_CrashDumpDecoding_H
