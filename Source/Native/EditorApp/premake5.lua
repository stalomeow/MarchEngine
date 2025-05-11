local cmd = require "Build/Commands"
local proj = require "Build/Projects"
local vcpkg = require "Build/Vcpkg"

project "EditorApp"
    kind "WindowedApp"

    proj.setup_cpp()
    vcpkg.setup()

    uses { "Engine", "DotNetRuntimeBinaries", "NsightAftermathBinaries", "ImGuizmo", "ImOGuizmo" }

    links {
        "Core",
        "Editor",
        "ShaderLab",
        "comdlg32.lib", -- 进度条控件
        "comctl32.lib", -- 进度条控件
    }

    postbuildcommands {
        -- 额外的 C# 程序集
        cmd.copy_file_to_build_target_dir(proj.get_csharp_binary_path("Core", "March.Core.runtimeconfig.json")),
        cmd.copy_file_to_build_target_dir(proj.get_csharp_binary_path("Core", "Newtonsoft.Json.dll")),
        cmd.copy_file_to_build_target_dir(proj.get_csharp_binary_path("Editor", "glTFLoader.dll")),
        cmd.copy_file_to_build_target_dir(proj.get_csharp_binary_path("ShaderLab", "Antlr4.Runtime.Standard.dll")),

        -- 额外的资源
        cmd.mirror_dir_to_build_target_dir(path.join(_MAIN_SCRIPT_DIR, "Resources"), "Resources"),
        cmd.mirror_dir_to_build_target_dir(path.join(_MAIN_SCRIPT_DIR, "Source/Shaders/Public"), "Shaders"),
    }

    filter "configurations:Debug"
        defines {
            -- 检查内存泄漏
            "_CRTDBG_MAP_ALLOC",

            -- 重定向部分资源的路径，路径使用 / 分隔
            string.format('ENGINE_SHADER_UNIX_PATH="%s"', path.translate(path.join(_MAIN_SCRIPT_DIR, "Source/Shaders/Public"), "/")),
            string.format('ENGINE_RESOURCE_UNIX_PATH="%s"', path.translate(path.join(_MAIN_SCRIPT_DIR, "Resources"), "/")),
        }
