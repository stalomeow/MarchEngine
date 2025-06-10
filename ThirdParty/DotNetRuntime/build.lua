local m = marchmodule {
    name = "DotNetRuntime",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

files {
    "Source/coreclr_delegates.h",
    "Source/hostfxr.h",
}

usage "PUBLIC"
    includedirs { "Source" }

    runtimedeps {
        ["Runtime/"] = "Binaries/%{cfg.platform}"
    }