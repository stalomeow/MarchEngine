local proj = require "Build/Projects"

project "ImGuizmo"
    kind "StaticLib"

    proj.setup_cpp_third_party()

    defines { "USE_IMGUI_API" }

    files {
        "Source/ImGuizmo.cpp",
        "Source/ImGuizmo.h",
    }

    usage "PUBLIC"
        uses { "ImGui" }

    usage "INTERFACE"
        includedirs { "Source" }
        links { "ImGuizmo" }