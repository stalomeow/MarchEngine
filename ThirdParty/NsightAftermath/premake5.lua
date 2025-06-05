local cmd = require "Build/Commands"
local proj = require "Build/Projects"

local function get_binary_path(ext)
    local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64
    local filename = string.format("GFSDK_Aftermath_Lib.%s.%s", arch, ext)
    return path.getabsolute(path.join("./Binaries", arch, filename))
end

project "NsightAftermath"
    kind "None" -- 纯头文件，不编译

    proj.setup_cpp_third_party()

    files {
        "Source/GFSDK_Aftermath.h",
        "Source/GFSDK_Aftermath_Defines.h",
        "Source/GFSDK_Aftermath_GpuCrashDump.h",
        "Source/GFSDK_Aftermath_GpuCrashDumpDecoding.h",
    }

    usage "INTERFACE"
        includedirs { "Source" }
        links { get_binary_path("lib") }

    usage "NsightAftermathBinaries"
        postbuildcommands {
            cmd.copy_file_to_build_target_dir(get_binary_path("dll"))
        }