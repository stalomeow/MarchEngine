local m = marchmodule {
    name = "RenderDoc",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

files {
    "include/renderdoc_app.h",
}

usage "PUBLIC"
    includedirs { "include" }