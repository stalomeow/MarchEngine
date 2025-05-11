local proj = require "Build/Projects"
local vcpkg = require "Build/Vcpkg"

project "Engine"
    kind "StaticLib"

    proj.setup_cpp()
    vcpkg.setup()

    usage "PUBLIC"
        uses { "DotNetRuntime", "ImGui", "NsightAftermath", "RenderDoc" }

        -- 允许给图形资源设置名称
        filter "configurations:Debug"
            defines { "ENABLE_GFX_DEBUG_NAME" }

    usage "INTERFACE"
        links {
            "Engine",
            "d3d12.lib",
            "dxgi.lib",
            "dxguid.lib",
        }

        -- C++ 通过导出函数来给 C# 调用，所以要确保所有函数都被链接到
        filter "action:vs*"
            linkoptions { "/WHOLEARCHIVE:Engine.lib" }
