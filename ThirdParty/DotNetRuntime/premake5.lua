local cmd = require "Build/Commands"

project "DotNetRuntime"
    kind "None" -- 纯头文件，不编译

    files {
        "Source/coreclr_delegates.h",
        "Source/hostfxr.h",
    }

    usage "INTERFACE"
        includedirs { "Source" }

    usage "DotNetRuntimeBinaries"
        postbuildcommands {
            cmd.mirror_dir_to_build_target_dir(path.getabsolute("Binaries/%{cfg.platform}"), "Runtime")
        }