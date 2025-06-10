local m = marchmodule {
    name = "NsightAftermath",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

files {
    "include/GFSDK_Aftermath.h",
    "include/GFSDK_Aftermath_Defines.h",
    "include/GFSDK_Aftermath_GpuCrashDump.h",
    "include/GFSDK_Aftermath_GpuCrashDumpDecoding.h",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64
local libName = string.format("GFSDK_Aftermath_Lib.%s.lib", arch)
local dllName = string.format("GFSDK_Aftermath_Lib.%s.dll", arch)

usage "PUBLIC"
    includedirs { "include" }

    runtimedeps {
        [dllName] = path.join("bin", arch, dllName),
    }

usage "INTERFACE"
    links { path.join("bin", arch, libName) }