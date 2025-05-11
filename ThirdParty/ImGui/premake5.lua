local proj = require "Build/Projects"

local function enable_freetype()
    -- freetype 是用 vcpkg 安装的
    local vcpkg = require "Build/Vcpkg"
    vcpkg.setup()

    defines { "IMGUI_ENABLE_FREETYPE" }

    files {
        "Source/misc/freetype/imgui_freetype.cpp",
        "Source/misc/freetype/imgui_freetype.h",
    }
end

project "ImGui"
    kind "StaticLib"

    proj.setup_cpp_third_party()

    files {
        "Source/imconfig.h",
        "Source/imgui.cpp",
        "Source/imgui.h",
        "Source/imgui_demo.cpp",
        "Source/imgui_draw.cpp",
        "Source/imgui_internal.h",
        "Source/imgui_tables.cpp",
        "Source/imgui_widgets.cpp",
        "Source/imstb_rectpack.h",
        "Source/imstb_textedit.h",
        "Source/imstb_truetype.h",

        -- 添加对 std::string 的支持
        "Source/misc/cpp/imgui_stdlib.cpp",
        "Source/misc/cpp/imgui_stdlib.h",

        -- Windows 支持
        "Source/backends/imgui_impl_win32.cpp",
        "Source/backends/imgui_impl_win32.h",
    }

    enable_freetype()

    usage "PUBLIC"
        includedirs {
            "Source",
            "Source/misc/cpp",
            "Source/backends",
        }

    usage "INTERFACE"
        links { "ImGui" }