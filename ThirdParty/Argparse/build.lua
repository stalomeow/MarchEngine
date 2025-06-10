local m = marchmodule {
    name = "Argparse",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/p-ranav/argparse/releases/tag/v3.2

files {
    "include/argparse/argparse.hpp",
}

usage "PUBLIC"
    includedirs { "include" }