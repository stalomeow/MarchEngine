local m = marchmodule {
    name = "ImOGuizmo",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

files {
    "Source/imoguizmo.hpp",
}

usage "PUBLIC"
    uses { "ImGui" }
    includedirs { "Source" }