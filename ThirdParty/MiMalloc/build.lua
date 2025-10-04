local m = marchmodule {
    name = "MiMalloc",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/microsoft/mimalloc/releases/tag/v3.1.4

-- x86 构建：
-- cmake -S . -B build_x86 -A Win32
-- cmake --build build_x86 --config Debug
-- cmake --build build_x86 --config Release

-- x64 构建：
-- cmake -S . -B build_x64 -A x64
-- cmake --build build_x64 --config Debug
-- cmake --build build_x64 --config Release

files {
    "include/**.h",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64

usage "PUBLIC"
    includedirs { "include" }

usage "INTERFACE"
    links {
        path.join("lib", arch, "%{cfg.buildcfg}/mimalloc.lib"),
    }
