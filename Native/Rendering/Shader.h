#pragma once

#include <vector>
#include <string>

namespace dx12demo
{
    struct ShaderProperty
    {

    };

    struct ShaderPass
    {
        std::string Name;
    };

    class Shader
    {

    private:
        std::string m_Name;
        std::vector<ShaderProperty> m_Properties;
        std::vector<ShaderPass> m_Passes;
    };
}
