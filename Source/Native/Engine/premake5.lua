local cmd = require "Build/Commands"
local proj = require "Build/Projects"
local vcpkg = require "Build/Vcpkg"

project "Engine"
    kind "StaticLib"

    proj.setup_cpp()
    vcpkg.include_headers()

    uses { "ImGui", "CoreCLRHost", "NsightAftermath", "RenderDoc" }

    usage "PUBLIC"
        filter "configurations:Debug"
            -- 允许给图形资源设置名称
            defines { "ENABLE_GFX_DEBUG_NAME" }

    usage "INTERFACE"
        links {
            "Engine",
            "d3d12.lib",
            "dxgi.lib",
            "dxguid.lib",
            proj.get_nsight_aftermath_binary_path("lib"),
        }

        postbuildcommands {
            cmd.copy_file_to_build_target_dir(proj.get_nsight_aftermath_binary_path("dll")),
        }

        filter "action:vs*"
            -- C++ 通过导出函数来给 C# 调用，所以要确保所有函数都被链接到
            linkoptions { "/WHOLEARCHIVE:Engine.lib" }
