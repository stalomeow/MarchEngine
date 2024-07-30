# CMakeList.txt : CMake project for dx12-demo, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>") 

project("dx12-demo")

find_package(directx-headers CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_definitions(-DUNICODE -D_UNICODE)
add_compile_definitions(ENABLE_GFX_DEBUG_NAME)

# Add source to this project's executable.
add_executable(dx12-demo WIN32 "App/WinMain.cpp" "Core/GameTimer.h" "Core/GameTimer.cpp" "Rendering/DxException.h" "Rendering/DxException.cpp" "Rendering/Mesh.hpp" "Core/MathHelper.h" "App/WinApplication.h"  "App/WinApplication.cpp" "Core/IEngine.h" "Rendering/RenderPipeline.h" "Rendering/RenderPipeline.cpp"   "Core/GameObject.h" "Core/IComponent.h" "Core/Transform.h" "Editor/GameEditor.h" "Editor/GameEditor.cpp" "Core/Debug.h" "Core/Debug.cpp" "Core/StringUtility.h" "Core/Scene.h"  "Rendering/Resource/GpuResource.h" "Rendering/Resource/GpuBuffer.h" "Rendering/Resource/GpuBuffer.cpp"  "Rendering/GfxManager.h" "Rendering/DescriptorHeap.h" "Rendering/DescriptorHeap.cpp" "Rendering/GfxManager.cpp" "Rendering/Command/CommandAllocatorPool.h" "Rendering/Command/CommandAllocatorPool.cpp" "Rendering/Command/CommandBuffer.h"  "Rendering/Command/CommandBuffer.cpp"  "Rendering/Resource/UploadHeapAllocator.h" "Rendering/Resource/UploadHeapAllocator.cpp" "Core/Scene.cpp" "Rendering/Light.h")
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids)
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectXShaderCompiler)
target_link_libraries(dx12-demo PRIVATE "d3dcompiler.lib" "D3D12.lib" "dxgi.lib")
target_link_libraries(dx12-demo PRIVATE imgui::imgui)

target_include_directories(dx12-demo PRIVATE ${PROJECT_SOURCE_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET dx12-demo PROPERTY CXX_STANDARD 17)
endif()

# TODO: Add tests and install targets if needed.
