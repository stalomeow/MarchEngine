local m = marchmodule {
    name = "DirectXShaderCompiler",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.8.2505

files {
    "inc/**.h",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64

usage "PUBLIC"
    includedirs { "inc" }

    runtimedeps {
        ["dxcompiler.dll"] = path.join("bin", arch, "dxcompiler.dll"),
        ["dxil.dll"] = path.join("bin", arch, "dxil.dll"),
    }

usage "INTERFACE"
    links {
        path.join("lib", arch, "dxcompiler.lib"),
        path.join("lib", arch, "dxil.lib"),
    }