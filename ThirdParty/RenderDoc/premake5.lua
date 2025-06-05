local proj = require "Build/Projects"

project "RenderDoc"
    kind "None" -- 纯头文件，不编译

    proj.setup_cpp_third_party()

    files {
        "Source/renderdoc_app.h",
    }

    usage "INTERFACE"
        includedirs { "Source" }