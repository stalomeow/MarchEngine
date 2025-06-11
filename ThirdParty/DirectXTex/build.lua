local m = marchmodule {
    name = "DirectXTex",
    type = "Native",
    kind = "None", -- 纯头文件，不编译
}

-- 当前版本：https://github.com/microsoft/DirectXTex/releases/tag/mar2025

-- x86 构建：
-- cmake -S . -B build_x86 -A Win32 -DBUILD_SHARED_LIBS=ON -DENABLE_OPENEXR_SUPPORT=OFF -DBUILD_DX11=OFF -DBUILD_DX12=ON -DBUILD_SAMPLE=OFF -DBUILD_TOOLS=OFF
-- cmake --build build_x86 --config Debug
-- cmake --build build_x86 --config Release
-- x64 构建：
-- cmake -S . -B build_x64 -A x64 -DBUILD_SHARED_LIBS=ON -DENABLE_OPENEXR_SUPPORT=OFF -DBUILD_DX11=OFF -DBUILD_DX12=ON -DBUILD_SAMPLE=OFF -DBUILD_TOOLS=OFF
-- cmake --build build_x64 --config Debug
-- cmake --build build_x64 --config Release

files {
    "include/DirectXTex.h",
    "include/DirectXTex.inl",
}

local arch = "%{cfg.architecture == 'x86_64' and 'x64' or cfg.architecture}" -- 将 x86_64 替换为 x64

usage "PUBLIC"
    uses { "DirectX12Agility", "DirectXMath" }
    includedirs { "include" }

    filter "configurations:Debug"
        runtimedeps {
            ["DirectXTex.dll"] = path.join("bin", arch, "Debug/DirectXTex.dll"),
            ["DirectXTex.pdb"] = path.join("bin", arch, "Debug/DirectXTex.pdb"),
        }

    filter "configurations:Release"
        runtimedeps {
            ["DirectXTex.dll"] = path.join("bin", arch, "Release/DirectXTex.dll"),
        }

usage "INTERFACE"
    filter "configurations:Debug"
        links {
            path.join("lib", arch, "Debug/DirectXTex.lib"),
        }

    filter "configurations:Release"
        links {
            path.join("lib", arch, "Release/DirectXTex.lib"),
        }
