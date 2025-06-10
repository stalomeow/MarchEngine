local m = marchmodule {
    name = "DirectXHeaders",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/microsoft/DirectX-Headers/releases/tag/v1.616.0

files {
    "include/**.h",
}

usage "PUBLIC"
    includedirs { "include" }