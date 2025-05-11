local cmd = require "Build/Commands"

local vcpkg = {}
local install_dir = path.join(_MAIN_SCRIPT_DIR, "Output", "Vcpkg")
local triplet = "x64-windows"

function vcpkg.install_packages()
    os.executef('vcpkg install --x-install-root="%s" --no-print-usage', install_dir)
end

local function get_path(...)
    return path.join(install_dir, triplet, ...)
end

function vcpkg.setup()
    includedirs { get_path("include") }

    -- 直接暴力链接所有库，开摆！

    filter { "kind:WindowedApp or ConsoleApp", "configurations:Debug" }
        links (os.matchfiles(get_path("debug", "lib", "*.lib")))

        postbuildcommands {
            cmd.copy_file_to_build_target_dir(get_path("debug", "bin", "*.dll")),
            cmd.copy_file_to_build_target_dir(get_path("debug", "bin", "*.pdb")),
        }

    filter { "kind:WindowedApp or ConsoleApp", "configurations:Release" }
        links (os.matchfiles(get_path("lib", "*.lib")))

        postbuildcommands {
            cmd.copy_file_to_build_target_dir(get_path("bin", "*.dll")),
        }

    -- 还原到默认的状态
    filter {}
end

return vcpkg