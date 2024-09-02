#include "Rendering/Material.h"

namespace dx12demo
{
    void Material::SetInt(std::string name, int32_t value)
    {
        Ints[name] = value;
    }

    void Material::SetFloat(std::string name, float value)
    {
        Floats[name] = value;
    }

    void Material::SetVector(std::string name, DirectX::XMFLOAT4 value)
    {
        Vectors[name] = value;
    }

    void Material::SetTexture(std::string name, Texture* texture)
    {
        Textures[name] = texture;
    }
}
