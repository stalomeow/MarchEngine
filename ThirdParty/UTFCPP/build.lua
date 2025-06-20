local m = marchmodule {
    name = "UTFCPP",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/nemtrif/utfcpp/releases/tag/v4.0.6

files {
    "source/**.h",
}

usage "PUBLIC"
    includedirs { "source" }
