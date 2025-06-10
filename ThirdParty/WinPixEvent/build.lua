local m = marchmodule {
    name = "WinPixEvent",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://www.nuget.org/packages/WinPixEventRuntime/1.0.240308001

files {
    "Include/WinPixEventRuntime/*.h",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64

usage "PUBLIC"
    includedirs { "Include/WinPixEventRuntime" }

    runtimedeps {
        ["WinPixEventRuntime.dll"] = path.join("bin", arch, "WinPixEventRuntime.dll"),
    }

usage "INTERFACE"
    links {
        path.join("bin", arch, "WinPixEventRuntime.lib"),
    }