local m = marchmodule {
    name = "FreeType",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.gz

-- x86 构建：
-- cmake -S . -B build_x86 -A Win32 -DBUILD_SHARED_LIBS=ON
-- cmake --build build_x86 --config Release

-- x64 构建：
-- cmake -S . -B build_x64 -A x64 -DBUILD_SHARED_LIBS=ON
-- cmake --build build_x64 --config Release

files {
    "include/**.h",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64

usage "PUBLIC"
    includedirs { "include" }

    runtimedeps {
        ["freetype.dll"] = path.join("bin", arch, "freetype.dll"),
    }

usage "INTERFACE"
    links {
        path.join("lib", arch, "freetype.lib"),
    }