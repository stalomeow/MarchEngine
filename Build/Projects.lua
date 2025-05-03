require("vstudio")

-- 只保留 Debug/Release，不在后面添加平台名称
premake.override(premake.vstudio, "projectPlatform", function(base, cfg)
    return cfg.buildcfg
end)

-- C# 项目永远都是 Any CPU
premake.override(premake.vstudio, "archFromConfig", function(base, cfg, win32)
    if premake.project.isdotnet(cfg.project) then
        return "Any CPU"
    end
    return base(cfg, win32)
end)

local proj = {}
local dotnet_version = "net9.0"

function proj.get_csharp_binary_path(project_name, binary_name)
    return path.join(_MAIN_SCRIPT_DIR, "Output/Binaries/Managed", project_name, "%{cfg.buildcfg}", dotnet_version, binary_name)
end

function proj.get_nsight_aftermath_binary_path(ext)
    local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64
    local filename = string.format("GFSDK_Aftermath_Lib.%s.%s", arch, ext)
    return path.join(_MAIN_SCRIPT_DIR, "Binaries/NsightAftermath", arch, filename)
end

local function include_dir_if_exists(dir_path)
    if os.isdir(dir_path) then
        includedirs { dir_path }
    end
end

function proj.setup_workspace()
    location (path.join(_MAIN_SCRIPT_DIR, "Output"))
    configurations { "Debug", "Release" }
    platforms { "Win64" }
end

function proj.setup_csharp()
    language "C#"
    kind "SharedLib"
    location (path.join(_MAIN_SCRIPT_DIR, "Source/Managed/%{prj.name}"))
end

function proj.setup_cpp()
    language "C++"
    cppdialect "C++17"

    rtti "Off"
    conformancemode "Off"
    characterset "Unicode"

    targetdir (path.join(_MAIN_SCRIPT_DIR, "Output/Binaries/Native/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"))
    objdir (path.join(_MAIN_SCRIPT_DIR, "Output/Intermediate/Native/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"))

    flags { "MultiProcessorCompile" }
    defines { "_UNICODE", "UNICODE", "NOMINMAX" }

    files {
        "**.h",
        "**.hpp",
        "**.cpp",
        "**.manifest",
        "**.rc",
    }

    pchheader "pch.h"
    pchsource "Private/pch.cpp"

    include_dir_if_exists "ThirdParty"
    include_dir_if_exists "Internal"

    vsprops {
        SDLCheck = "true",
        VcpkgEnabled = "false", -- Visual Studio 的 Vcpkg 和 premake5 一起用似乎有 bug，所以强制关闭它，改用我们自己的实现
    }

    filter "configurations:Debug"
        defines { "_DEBUG" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "NDEBUG" }
        runtime "Release"
        symbols "On"
        optimize "Full"

    filter "platforms:Win64"
        system "Windows"
        architecture "x86_64"

    filter "action:vs*"
        buildoptions { "/utf-8" }

    filter {}
        usage "PUBLIC"
            include_dir_if_exists "Public"

    -- 还原到默认的状态
    usage "PRIVATE"
end

return proj