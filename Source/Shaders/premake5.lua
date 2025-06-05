local proj = require "Build/Projects"

project "Shaders"
    kind "SharedItems"

    proj.setup_shader()

    files {
        "**.hlsl",
        "**.shader",
        "**.compute",
    }
