local m = marchmodule {
    name = "CoreNative",
    type = "Native",
    kind = "StaticLib",
    wholeArchive = true, -- C++ 通过导出函数来给 C# 调用，所以要确保所有函数都被链接到
}

usage "PUBLIC"
    uses {
        "DirectX12Agility",
        "DirectXMath",
        "DirectXShaderCompiler",
        "DirectXTex",
        "DotNetRuntime",
        "Fmt",
        "NsightAftermath",
        "RenderDoc",
        "UTFCPP",
        "WinPixEvent",
    }

    -- 允许给图形资源设置名称
    filter "configurations:Debug"
        defines { "ENABLE_GFX_DEBUG_NAME" }