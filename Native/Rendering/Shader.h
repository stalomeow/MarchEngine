#pragma once

#include <vector>
#include <string>

namespace dx12demo
{
    struct ShaderCompileResult
    {

    };

    class Shader
    {
    public:
        static void Compile(const std::string& filename, const std::string& entrypoint, const std::string& targetProfile);

    };
}
