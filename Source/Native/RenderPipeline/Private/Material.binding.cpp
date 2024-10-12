#include "Material.h"
#include "InteropServices.h"
#include "Shader.h"

NATIVE_EXPORT_AUTO Material_New()
{
    retcs DBG_NEW Material();
}

NATIVE_EXPORT_AUTO Material_Delete(cs<Material*> pMaterial)
{
    delete pMaterial;
}

NATIVE_EXPORT_AUTO Material_Reset(cs<Material*> pMaterial)
{
    pMaterial->Reset();
}

NATIVE_EXPORT_AUTO Material_SetShader(cs<Material*> pMaterial, cs<Shader*> pShader)
{
    pMaterial->SetShader(pShader);
}

NATIVE_EXPORT_AUTO Material_SetInt(cs<Material*> pMaterial, cs_string name, cs_int value)
{
    pMaterial->SetInt(name, value);
}

NATIVE_EXPORT_AUTO Material_SetFloat(cs<Material*> pMaterial, cs_string name, cs_float value)
{
    pMaterial->SetFloat(name, value);
}

NATIVE_EXPORT_AUTO Material_SetVector(cs<Material*> pMaterial, cs_string name, cs_vec4 value)
{
    pMaterial->SetVector(name, value);
}

NATIVE_EXPORT_AUTO Material_SetColor(cs<Material*> pMaterial, cs_string name, cs_color value)
{
    pMaterial->SetColor(name, value);
}

NATIVE_EXPORT_AUTO Material_SetTexture(cs<Material*> pMaterial, cs_string name, cs<GfxTexture*> pTexture)
{
    pMaterial->SetTexture(name, pTexture);
}

NATIVE_EXPORT_AUTO Material_GetInt(cs<Material*> pMaterial, cs_string name, cs<cs_int*> outValue)
{
    int32_t v = *outValue;
    bool ret = pMaterial->GetInt(name, &v);
    outValue->assign(v);
    retcs ret;
}

NATIVE_EXPORT_AUTO Material_GetFloat(cs<Material*> pMaterial, cs_string name, cs<cs_float*> outValue)
{
    float v = *outValue;
    bool ret = pMaterial->GetFloat(name, &v);
    outValue->assign(v);
    retcs ret;
}

NATIVE_EXPORT_AUTO Material_GetVector(cs<Material*> pMaterial, cs_string name, cs<cs_vec4*> outValue)
{
    XMFLOAT4 v = *outValue;
    bool ret = pMaterial->GetVector(name, &v);
    outValue->assign(v);
    retcs ret;
}

NATIVE_EXPORT_AUTO Material_GetColor(cs<Material*> pMaterial, cs_string name, cs<cs_color*> outValue)
{
    XMFLOAT4 v = *outValue;
    bool ret = pMaterial->GetColor(name, &v);
    outValue->assign(v);
    retcs ret;
}

NATIVE_EXPORT_AUTO Material_HasTextureProperty(cs<Material*> pMaterial, cs_string name)
{
    GfxTexture* temp = nullptr;
    retcs pMaterial->GetTexture(name, &temp);
}

struct IntProperty
{
    cs_string Name;
    cs_int Value;
};

NATIVE_EXPORT_AUTO Material_GetAllInts(cs<Material*> pMaterial)
{
    const std::unordered_map<std::string, int32_t> rawInts = MaterialInternalUtility::GetRawInts(pMaterial);
    std::unordered_map<std::string, int32_t> allValues(rawInts.begin(), rawInts.end());

    for (auto& p : pMaterial->GetShader()->Properties)
    {
        if (p.second.Type == ShaderPropertyType::Int && !allValues.count(p.first))
        {
            allValues[p.first] = p.second.DefaultInt;
        }
    }

    cs_array<IntProperty> props{};
    props.assign(static_cast<int32_t>(allValues.size()));

    int32_t i = 0;
    for (auto& p : allValues)
    {
        IntProperty& dst = props[i++];
        dst.Name.assign(p.first);
        dst.Value.assign(p.second);
    }

    retcs props;
}

struct FloatProperty
{
    cs_string Name;
    cs_float Value;
};

NATIVE_EXPORT_AUTO Material_GetAllFloats(cs<Material*> pMaterial)
{
    const std::unordered_map<std::string, float> rawFloats = MaterialInternalUtility::GetRawFloats(pMaterial);
    std::unordered_map<std::string, float> allValues(rawFloats.begin(), rawFloats.end());

    for (auto& p : pMaterial->GetShader()->Properties)
    {
        if (p.second.Type == ShaderPropertyType::Float && !allValues.count(p.first))
        {
            allValues[p.first] = p.second.DefaultFloat;
        }
    }

    cs_array<FloatProperty> props{};
    props.assign(static_cast<int32_t>(allValues.size()));

    int32_t i = 0;
    for (auto& p : allValues)
    {
        FloatProperty& dst = props[i++];
        dst.Name.assign(p.first);
        dst.Value.assign(p.second);
    }

    retcs props;
}

struct VectorProperty
{
    cs_string Name;
    cs_vec4 Value;
};

NATIVE_EXPORT_AUTO Material_GetAllVectors(cs<Material*> pMaterial)
{
    const std::unordered_map<std::string, XMFLOAT4> rawVectors = MaterialInternalUtility::GetRawVectors(pMaterial);
    std::unordered_map<std::string, XMFLOAT4> allValues(rawVectors.begin(), rawVectors.end());

    for (auto& p : pMaterial->GetShader()->Properties)
    {
        if (p.second.Type == ShaderPropertyType::Vector && !allValues.count(p.first))
        {
            allValues[p.first] = p.second.DefaultVector;
        }
    }

    cs_array<VectorProperty> props{};
    props.assign(static_cast<int32_t>(allValues.size()));

    int32_t i = 0;
    for (auto& p : allValues)
    {
        VectorProperty& dst = props[i++];
        dst.Name.assign(p.first);
        dst.Value.assign(p.second);
    }

    retcs props;
}

struct ColorProperty
{
    cs_string Name;
    cs_color Value;
};

NATIVE_EXPORT_AUTO Material_GetAllColors(cs<Material*> pMaterial)
{
    const std::unordered_map<std::string, XMFLOAT4> rawColors = MaterialInternalUtility::GetRawColors(pMaterial);
    std::unordered_map<std::string, XMFLOAT4> allValues(rawColors.begin(), rawColors.end());

    for (auto& p : pMaterial->GetShader()->Properties)
    {
        if (p.second.Type == ShaderPropertyType::Color && !allValues.count(p.first))
        {
            allValues[p.first] = p.second.DefaultColor;
        }
    }

    cs_array<ColorProperty> props{};
    props.assign(static_cast<int32_t>(allValues.size()));

    int32_t i = 0;
    for (auto& p : allValues)
    {
        ColorProperty& dst = props[i++];
        dst.Name.assign(p.first);
        dst.Value.assign(p.second);
    }

    retcs props;
}

struct TextureProperty
{
    cs_string Name;
    cs_ptr<GfxTexture> Value;
};

NATIVE_EXPORT_AUTO Material_GetAllTextures(cs<Material*> pMaterial)
{
    const std::unordered_map<std::string, GfxTexture*> rawTextures = MaterialInternalUtility::GetRawTextures(pMaterial);
    std::unordered_map<std::string, GfxTexture*> allValues(rawTextures.begin(), rawTextures.end());

    for (auto& p : pMaterial->GetShader()->Properties)
    {
        if (p.second.Type == ShaderPropertyType::Texture && !allValues.count(p.first))
        {
            allValues[p.first] = p.second.GetDefaultTexture();
        }
    }

    cs_array<TextureProperty> props{};
    props.assign(static_cast<int32_t>(allValues.size()));

    int32_t i = 0;
    for (auto& p : allValues)
    {
        TextureProperty& dst = props[i++];
        dst.Name.assign(p.first);
        dst.Value.assign(p.second);
    }

    retcs props;
}
