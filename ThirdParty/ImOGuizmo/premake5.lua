local proj = require "Build/Projects"

project "ImOGuizmo"
    kind "None" -- 纯头文件，不编译

    proj.setup_cpp_third_party()

    files {
        "Source/imoguizmo.hpp",
    }

    usage "INTERFACE"
        includedirs { "Source" }