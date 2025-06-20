local m = marchmodule {
    name = "DotNetRuntime",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

files {
    "Source/coreclr_delegates.h",
    "Source/hostfxr.h",
}

-- https://learn.microsoft.com/en-us/dotnet/core/runtime-config/
-- https://github.com/dotnet/sdk/blob/main/documentation/specs/runtime-configuration-file.md
local runtimeConfigFile = "runtimeconfig.json"

local function getDotNetVersion()
    local config, err = json.decode(io.readfile(runtimeConfigFile))
    if err ~= nil then
        error("Failed to read .NET " .. runtimeConfigFile .. ": " .. err)
    end
    return config.runtimeOptions.framework.version
end

usage "PUBLIC"
    includedirs { "Source" }

    defines {
        string.format('DOTNET_RUNTIME_CONFIG_PATH="%s"', runtimeConfigFile),
        string.format('DOTNET_HOSTFXR_PATH="Runtime/host/fxr/%s/hostfxr.dll"', getDotNetVersion()),
    }

    runtimedeps {
        ["Runtime/"] = "Binaries/%{cfg.platform}",
        [runtimeConfigFile] = runtimeConfigFile,
    }
