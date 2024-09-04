# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/source-charset:utf-8>") 

find_package(directx-headers CONFIG REQUIRED)
find_package(directx-dxc CONFIG REQUIRED)
find_package(directxtex CONFIG REQUIRED)
# find_package(directxtk12 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_definitions(-DUNICODE -D_UNICODE)

file(GLOB_RECURSE SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h" "*.cpp" "*.hpp")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

# Add source to this project's executable.
add_executable(dx12-demo WIN32 ${SOURCES} "${CMAKE_SOURCE_DIR}/.editorconfig")
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectX-Headers Microsoft::DirectX-Guids)
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectXShaderCompiler)
target_link_libraries(dx12-demo PRIVATE "d3dcompiler.lib" "D3D12.lib" "dxgi.lib")
target_link_libraries(dx12-demo PRIVATE imgui::imgui)
target_link_libraries(dx12-demo PRIVATE "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/8.0.8/runtimes/win-x64/native/nethost.lib")
# target_link_libraries(dx12-demo PRIVATE Microsoft::DirectXTK12)
target_link_libraries(dx12-demo PRIVATE Microsoft::DirectXTex)

# Works correctly for both single and multi-config generators
target_compile_definitions(dx12-demo PRIVATE
  $<$<CONFIG:Debug>:ENABLE_GFX_DEBUG_NAME>
)

add_custom_command(TARGET dx12-demo POST_BUILD 
               COMMAND ${CMAKE_COMMAND} -E copy_if_different
               "C:/Program Files/dotnet/packs/Microsoft.NETCore.App.Host.win-x64/8.0.8/runtimes/win-x64/native/nethost.dll"
               "C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/dxil.dll"
               $<TARGET_FILE_DIR:dx12-demo>)

# C# 程序集
add_dependencies(dx12-demo Core)
add_dependencies(dx12-demo Editor)
add_custom_command(TARGET dx12-demo POST_BUILD 
               COMMAND ${CMAKE_COMMAND} -E copy_if_different
               "${CMAKE_SOURCE_DIR}/out/build/Managed/Core/bin/$<CONFIG>/net8.0/DX12Demo.Core.dll" # 会自动 copy pdb
               "${CMAKE_SOURCE_DIR}/out/build/Managed/Core/bin/$<CONFIG>/net8.0/DX12Demo.Core.runtimeconfig.json"
               "${CMAKE_SOURCE_DIR}/out/build/Managed/Core/bin/$<CONFIG>/net8.0/Newtonsoft.Json.dll"
               "${CMAKE_SOURCE_DIR}/out/build/Managed/Editor/bin/$<CONFIG>/net8.0/DX12Demo.Editor.dll" # 会自动 copy pdb
               $<TARGET_FILE_DIR:dx12-demo>)

target_include_directories(dx12-demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT dx12-demo)
set_target_properties(dx12-demo PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:dx12-demo>)

set_property(TARGET dx12-demo PROPERTY CXX_STANDARD 17)
set_property(TARGET dx12-demo PROPERTY CXX_STANDARD_REQUIRED ON)
