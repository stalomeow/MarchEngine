local m = marchmodule {
    name = "CoreNative",
    type = "Native",
    kind = "StaticLib",
}

usage "PUBLIC"
    uses {
        "DirectXHeaders",
        "DirectXMath",
        "DirectXShaderCompiler",
        "DirectXTex",
        "DotNetRuntime",
        "Fmt",
        "NsightAftermath",
        "RenderDoc",
        "WinPixEvent",
    }

    -- 允许给图形资源设置名称
    filter "configurations:Debug"
        defines { "ENABLE_GFX_DEBUG_NAME" }

usage "INTERFACE"
    links {
        "CoreNative",
        "d3d12.lib",
        "dxgi.lib",
        "dxguid.lib",
    }

    -- C++ 通过导出函数来给 C# 调用，所以要确保所有函数都被链接到
    filter "action:vs*"
        linkoptions { "/WHOLEARCHIVE:CoreNative.lib" }