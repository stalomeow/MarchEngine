if not os.istarget("windows") or not (os.hostarch() == "x86_64" or (os.hostarch() == "x86" and os.is64bit())) then
    print("This project is only supported on x86-64 Windows now.")
    os.exit(1)
end

local proj = require "Build/Projects"
local vcpkg = require "Build/Vcpkg"

newaction {
    trigger = "install",
    description = "Install packages",
    execute = function ()
        vcpkg.install_packages()
    end
}

workspace "MarchEngine"
    proj.setup_workspace()
    startproject "EditorApp"

include "Source/Shaders"

group "Native"
    include "Source/Native/Engine"
    include "Source/Native/EditorApp"

group "Managed"
    externalproject "Binding"
        uuid "E5352438-5F3A-4E5E-A160-F66541E8D2CD"
        proj.setup_csharp()

    externalproject "Core"
        uuid "EC2990C8-B5DC-4E58-911A-BE1433E7750A"
        proj.setup_csharp()

    externalproject "Editor"
        uuid "CFC46F16-60D5-475F-A7D9-1E1869DE3064"
        proj.setup_csharp()

    externalproject "ShaderLab"
        uuid "123DAE4E-49F3-CED1-0410-D6D8B4BE9F8E"
        proj.setup_csharp()
