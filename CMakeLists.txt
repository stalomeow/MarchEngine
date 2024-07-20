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

# Add source to this project's executable.
add_executable(dx12-demo WIN32 "main.cpp" "core/timer.h" "core/timer.cpp" "core/app.h" "core/app.cpp" "core/dx_exception.h" "core/dx_exception.cpp" "core/upload_buffer.hpp"  "core/mesh.hpp"  "core/math_helper.h")
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids)
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectXShaderCompiler)
target_link_libraries(dx12-demo PRIVATE "d3dcompiler.lib" "D3D12.lib" "dxgi.lib")
target_link_libraries(dx12-demo PRIVATE imgui::imgui)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET dx12-demo PROPERTY CXX_STANDARD 17)
endif()

# TODO: Add tests and install targets if needed.
