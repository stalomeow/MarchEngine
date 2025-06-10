local m = marchmodule {
    name = "DirectXMath",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://www.nuget.org/packages/directxmath/2025.4.3.1

files {
    "include/**.h",
    "include/**.inl",
}

usage "PUBLIC"
    includedirs { "include" }