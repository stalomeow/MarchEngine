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

local function setup_common_cpp()
    language "C++"
    cppdialect "C++17"

    rtti "Off"
    conformancemode "Off"
    characterset "Unicode"

    flags { "MultiProcessorCompile" }
    defines { "_UNICODE", "UNICODE", "NOMINMAX" }

    vsprops {
        SDLCheck = "true",
        VcpkgEnabled = "false", -- Visual Studio 的 Vcpkg 和 premake5 一起用似乎有 bug，所以强制关闭它，改用我们自己的实现
    }

    filter "configurations:Debug"
        defines { "DEBUG", "_DEBUG" }
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

    -- 还原到默认的状态
    filter {}
end

function proj.setup_cpp()
    setup_common_cpp()

    targetdir (path.join(_MAIN_SCRIPT_DIR, "Output/Binaries/Native/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"))
    objdir (path.join(_MAIN_SCRIPT_DIR, "Output/Intermediate/Native/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"))

    files {
        "**.h",
        "**.hpp",
        "**.cpp",
        "**.manifest",
        "**.rc",
    }

    include_dir_if_exists "Private"

    if os.isfile("Private/pch.h") and os.isfile("Private/pch.cpp") then
        pchheader "pch.h"
        pchsource "Private/pch.cpp"
    end

    local proj_name = project().name

    usage "PRIVATE"
        defines {
            string.format("%s_API=__declspec(dllexport)", string.upper(proj_name))
        }

    usage "INTERFACE"
        defines {
            string.format("%s_API=__declspec(dllimport)", string.upper(proj_name))
        }

    usage "PUBLIC"
        include_dir_if_exists "Public"

    -- 还原到默认的状态
    usage "PRIVATE"
end

function proj.setup_cpp_third_party()
    setup_common_cpp()

    targetdir (path.join(_MAIN_SCRIPT_DIR, "Output/Binaries/ThirdParty/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"))
    objdir (path.join(_MAIN_SCRIPT_DIR, "Output/Intermediate/ThirdParty/%{prj.name}/%{cfg.buildcfg}/%{cfg.platform}"))
end

return proj