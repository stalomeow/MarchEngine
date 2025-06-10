local m = marchmodule {
    name = "ImGuizmo",
    type = "Native",
    kind = "StaticLib",
}

defines { "USE_IMGUI_API" }

files {
    "Source/ImGuizmo.cpp",
    "Source/ImGuizmo.h",
}

usage "PUBLIC"
    uses { "ImGui" }
    includedirs { "Source" }

usage "INTERFACE"
    links { "ImGuizmo" }