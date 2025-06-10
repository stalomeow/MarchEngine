local m = marchmodule {
    name = "Fmt",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/fmtlib/fmt/releases/tag/11.2.0

-- x86 构建：
-- cmake -S . -B build_x86 -A Win32 -DFMT_TEST=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_STANDARD=17 -DFMT_UNICODE=ON
-- cmake --build build_x86 --config Debug
-- cmake --build build_x86 --config Release

-- x64 构建：
-- cmake -S . -B build_x64 -A x64 -DFMT_TEST=OFF -DBUILD_SHARED_LIBS=ON -DCMAKE_CXX_STANDARD=17 -DFMT_UNICODE=ON
-- cmake --build build_x64 --config Debug
-- cmake --build build_x64 --config Release

files {
    "include/fmt/*.h",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64

usage "PUBLIC"
    includedirs { "include" }

    defines {
        "FMT_SHARED=1",
    }

    filter "configurations:Debug"
        runtimedeps {
            ["fmtd.dll"] = path.join("bin", arch, "Debug/fmtd.dll"),
            ["fmtd.pdb"] = path.join("bin", arch, "Debug/fmtd.pdb"),
        }

    filter "configurations:Release"
        runtimedeps {
            ["fmt.dll"] = path.join("bin", arch, "Release/fmt.dll"),
        }

usage "INTERFACE"
    filter "configurations:Debug"
        links {
            path.join("lib", arch, "Debug/fmtd.lib"),
        }

    filter "configurations:Release"
        links {
            path.join("lib", arch, "Release/fmt.lib"),
        }
