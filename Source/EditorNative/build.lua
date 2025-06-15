local m = marchmodule {
    name = "EditorNative",
    type = "Native",
    kind = "WindowedApp",
}

debugdir(m.binaryDir)
debuggertype "NativeWithManagedCore" -- 使用 Mixed (.NET Core) Debugger 

uses {
    "CoreManaged",
    "EditorManaged",

    "CoreNative",

    "Argparse",
    "DotNetRuntime",
    "NsightAftermath",
    "ImGui",
    "ImGuizmo",
    "ImOGuizmo",
}

links {
    "comdlg32.lib", -- 进度条控件
    "comctl32.lib", -- 进度条控件
}

runtimedeps {
    ["Resources/"] = path.join(ENGINE_DIR, "Resources"),
    ["Shaders/"] = path.join(ENGINE_DIR, "Source/Shaders/Public"),
}

filter "configurations:Debug"
    defines {
        -- 检查内存泄漏
        "_CRTDBG_MAP_ALLOC",

        string.format('EDITOR_APP_NAME="%s"', m.name),
        string.format('EDITOR_APP_VERSION="%s"', "1.0"),

        -- 重定向部分资源的路径，路径使用 / 分隔
        string.format('ENGINE_SHADER_UNIX_PATH="%s"', path.translate(path.join(ENGINE_DIR, "Source/Shaders/Public"), "/")),
        string.format('ENGINE_RESOURCE_UNIX_PATH="%s"', path.translate(path.join(ENGINE_DIR, "Resources"), "/")),
    }
