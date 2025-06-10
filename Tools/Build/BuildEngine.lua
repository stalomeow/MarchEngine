if not os.istarget("windows") or not (os.hostarch() == "x86_64" or (os.hostarch() == "x86" and os.is64bit())) then
    error("The engine is only supported on x86-64 Windows now")
end

ENGINE_DIR = path.normalize(path.join(_MAIN_SCRIPT_DIR, "../.."))

local function managedModule(m)
    language "C#"
    csversion "13.0" -- C# 13.0
    clr "Unsafe"     -- 允许 unsafe 代码

    if m.kind == "SourceGenerator" then
        dotnetframework "netstandard2.0"

        nuget {
            "Microsoft.CodeAnalysis.CSharp:4.12.0",
            "Microsoft.CodeAnalysis.Analyzers:3.3.4",
        }
    else
        dotnetframework "net9.0"

        vsprops {
            ImplicitUsings = "enable",
            Nullable = "enable",
            CopyLocalLockFileAssemblies = "true",
        }
    end

    files  { "**.cs" }
    vpaths { ["*"] = path.getabsolute(".") }

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        symbols "On"
        optimize "Full"

    filter {} -- 还原到默认的状态

    usage "INTERFACE"
        links { m.name }
end

local function nativeModule(m)
    language "C++"
    cppdialect "C++17"

    rtti "Off"
    conformancemode "Off"
    characterset "Unicode"

    flags { "MultiProcessorCompile" }
    defines { "_UNICODE", "UNICODE", "NOMINMAX" }

    vsprops {
        SDLCheck = "true",
        VcpkgEnabled = "false",
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

    filter {} -- 还原到默认的状态

    if not m.isThirdParty then
        files {
            "**.h",
            "**.hpp",
            "**.c",
            "**.cc",
            "**.cpp",
            "**.cxx",
            "**.manifest",
            "**.rc",
        }

        if os.isdir("Private") then
            includedirs { "Private" }
        end

        if os.isfile("Private/pch.h") and os.isfile("Private/pch.cpp") then
            pchheader "pch.h"
            pchsource "Private/pch.cpp"
        end

        if m.kind == "StaticLib" then
            usage "PUBLIC"
                defines { string.format("%s_API", string.upper(m.name)) }
        elseif m.kind == "SharedLib" then
            defines { string.format("%s_API=__declspec(dllexport)", string.upper(m.name)) }
            usage "INTERFACE"
                defines { string.format("%s_API=__declspec(dllimport)", string.upper(m.name)) }
        end

        if os.isdir("Public") then
            usage "PUBLIC"
                includedirs { "Public" }
        end
    end

    if m.kind == "StaticLib" then
        usage "INTERFACE"
            links { m.name }

            if m.wholeArchive then
                filter "action:vs*"
                    -- TODO 这里没考虑 targetprefix targetname targetsuffix targetextension
                    linkoptions { string.format("/WHOLEARCHIVE:%s.lib", m.name) }
            end
    end
end

local function shaderModule(m)
    files {
        "**.hlsl",
        "**.shader",
        "**.compute",
    }
end

local thirdPartyModuleGroup = false

function marchmodule(config)
    local m = {
        name            = config.name,
        type            = config.type,
        kind            = config.kind,
        wholeArchive    = config.wholeArchive or false,
        isThirdParty    = thirdPartyModuleGroup,
        projectFileDir  = path.join(ENGINE_DIR, "Generated/ProjectFiles"),
        binaryDir       = path.join(ENGINE_DIR, "Generated/Binaries", config.name, "%{cfg.buildcfg}/%{cfg.platform}"),
        intermediateDir = path.join(ENGINE_DIR, "Generated/Intermediate", config.name, "%{cfg.buildcfg}/%{cfg.platform}"),
    }

    project(m.name)

        kind(m.kind)

        location(m.projectFileDir)
        targetdir(m.binaryDir)
        objdir(m.intermediateDir)

        files {
            "build.lua",
        }

        if m.type == "Managed" then
            managedModule(m)
        elseif m.type == "Native" then
            nativeModule(m)
        elseif m.type == "Shader" then
            shaderModule(m)
        else
            error("Unsupported march module type: " .. m.type)
        end

    project(m.name) -- 还原到默认的状态
    return m
end

newaction {
    trigger     = "clean",
    description = "Remove all generated files",
    execute     = function()
        os.rmdir(path.join(ENGINE_DIR, "Generated"))
        os.remove(path.join(ENGINE_DIR, "MarchEngine.sln"))
    end
}

newaction {
    trigger     = "touch",
    description = "Touch files",
    execute     = function()
        for _, f in ipairs(_ARGS) do
            local ok, err = os.touchfile(f)
            if not ok then
                error(err)
            end
        end
    end
}

workspace "MarchEngine"
    location(ENGINE_DIR)

    configurations { "Debug", "Release" }
    platforms { "Win64" }
    startproject "EditorNative"

    configfiles {
        path.join(ENGINE_DIR, ".editorconfig"),
        path.join(ENGINE_DIR, ".gitattributes"),
        path.join(ENGINE_DIR, ".gitignore"),
    }

local function discoverModules(name)
    group(name)

    local pattern = path.join(ENGINE_DIR, name, "**/build.lua")
    for _, f in ipairs(os.matchfiles(pattern)) do
        include(f)
    end
end

thirdPartyModuleGroup = false
discoverModules("Source")

thirdPartyModuleGroup = true
discoverModules("ThirdParty")