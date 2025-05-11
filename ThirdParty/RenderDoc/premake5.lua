project "RenderDoc"
    kind "None" -- 纯头文件，不编译

    files {
        "Source/renderdoc_app.h",
    }

    usage "INTERFACE"
        includedirs { "Source" }