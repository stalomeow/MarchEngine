project "ImOGuizmo"
    kind "None" -- 纯头文件，不编译

    files {
        "Source/imoguizmo.hpp",
    }

    usage "INTERFACE"
        includedirs { "Source" }