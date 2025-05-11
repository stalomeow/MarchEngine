local proj = require "Build/Projects"
local vcpkg = require "Build/Vcpkg"

project "ImGui"
    kind "StaticLib"

    proj.setup_cpp_third_party()
    vcpkg.include_headers()

    defines { "IMGUI_ENABLE_FREETYPE" }

    files {
        "ImGui/imconfig.h",
        "ImGui/imgui.cpp",
        "ImGui/imgui.h",
        "ImGui/imgui_demo.cpp",
        "ImGui/imgui_draw.cpp",
        "ImGui/imgui_internal.h",
        "ImGui/imgui_tables.cpp",
        "ImGui/imgui_widgets.cpp",

        "ImGui/misc/cpp/imgui_stdlib.cpp",
        "ImGui/misc/cpp/imgui_stdlib.h",

        "ImGui/misc/freetype/imgui_freetype.cpp",
        "ImGui/misc/freetype/imgui_freetype.h",

        "ImGui/backends/imgui_impl_win32.cpp",
        "ImGui/backends/imgui_impl_win32.h",
    }

    usage "PUBLIC"
        includedirs {
            "ImGui",
            "ImGui/misc/cpp",
            "ImGui/backends",
        }

    usage "INTERFACE"
        links { "ImGui" }

project "ImGuizmo"
    kind "StaticLib"

    proj.setup_cpp_third_party()

    uses { "ImGui" }
    defines { "USE_IMGUI_API" }

    files {
        "ImGuizmo/GraphEditor.cpp",
        "ImGuizmo/GraphEditor.h",
        "ImGuizmo/ImCurveEdit.cpp",
        "ImGuizmo/ImCurveEdit.h",
        "ImGuizmo/ImGradient.cpp",
        "ImGuizmo/ImGradient.h",
        "ImGuizmo/ImGuizmo.cpp",
        "ImGuizmo/ImGuizmo.h",
        "ImGuizmo/ImSequencer.cpp",
        "ImGuizmo/ImSequencer.h",
        "ImGuizmo/ImZoomSlider.h",
    }

    usage "PUBLIC"
        includedirs { "ImGuizmo" }

    usage "INTERFACE"
        links { "ImGuizmo" }

project "ImOGuizmo"
    kind "SharedItems"
    language "C++"

    files {
        "ImOGuizmo/**.hpp",
    }

    usage "PUBLIC"
        includedirs { "ImOGuizmo" }

    usage "INTERFACE"
        links { "ImOGuizmo" }

project "CoreCLRHost"
    kind "SharedItems"
    language "C++"

    files {
        "CoreCLRHost/**.h",
    }

    usage "PUBLIC"
        includedirs { "CoreCLRHost" }

    usage "INTERFACE"
        links { "CoreCLRHost" }

project "NsightAftermath"
    kind "SharedItems"
    language "C++"

    files {
        "NsightAftermath/**.h",
    }

    usage "PUBLIC"
        includedirs { "NsightAftermath" }

    usage "INTERFACE"
        links { "NsightAftermath" }

project "RenderDoc"
    kind "SharedItems"
    language "C++"

    files {
        "RenderDoc/**.h",
    }

    usage "PUBLIC"
        includedirs { "RenderDoc" }

    usage "INTERFACE"
        links { "RenderDoc" }
