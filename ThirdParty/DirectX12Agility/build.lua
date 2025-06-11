local m = marchmodule {
    name = "DirectX12Agility",
    type = "Native",
    kind = "StaticLib",
}

-- https://devblogs.microsoft.com/directx/directx12agility/
-- https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/
-- 当前版本：https://www.nuget.org/packages/Microsoft.Direct3D.D3D12/1.616.0

files {
    "include/**.h",
    "include/**.hpp",
    "src/d3dx12/d3dx12_property_format_table.cpp",
}

DX12_AGILITY_BIN_ARCH = {
    ["x86_64"] = "x64",
    ["x86"] = "win32",
}

local arch = "%{DX12_AGILITY_BIN_ARCH[cfg.architecture] or cfg.architecture}"

usage "INTERFACE"
    links {
        "d3d12.lib",
        "dxgi.lib",
        "dxguid.lib",
    }

usage "PUBLIC"
    includedirs { "include", "include/d3dx12" }

    runtimedeps {
        ["D3D12/D3D12Core.dll"] = path.join("bin", arch, "D3D12Core.dll"),
        ["D3D12/d3d12SDKLayers.dll"] = path.join("bin", arch, "d3d12SDKLayers.dll"),
    }

    filter "configurations:Debug"
        runtimedeps {
            ["D3D12/D3D12Core.pdb"] = path.join("bin", arch, "D3D12Core.pdb"),
            ["D3D12/d3d12SDKLayers.pdb"] = path.join("bin", arch, "d3d12SDKLayers.pdb"),
        }