# CMakeList.txt : CMake project for dx12-demo, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.20)

project("dx12-demo")

include_external_msproject(Core "${CMAKE_SOURCE_DIR}/Managed/Core/Core.csproj" PLATFORM AnyCPU)
include_external_msproject(Binding "${CMAKE_SOURCE_DIR}/Managed/Binding/Binding.csproj" PLATFORM AnyCPU)

add_subdirectory(Native)

# TODO: Add tests and install targets if needed.
