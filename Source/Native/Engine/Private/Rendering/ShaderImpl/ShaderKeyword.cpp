#include "pch.h"
#include "Engine/Rendering/ShaderImpl/ShaderKeyword.h"
#include "Engine/Rendering/ShaderImpl/ShaderUtils.h"
#include "Engine/StringUtils.h"
#include "Engine/Debug.h"
#include <stdexcept>

namespace march
{
    bool ShaderKeywordSpace::RegisterKeyword(const std::string& keyword)
    {
        return RegisterKeyword(ShaderUtils::GetIdFromString(keyword));
    }

    bool ShaderKeywordSpace::RegisterKeyword(int32 keywordId)
    {
        if (m_KeywordIndexMap.count(keywordId) == 0)
        {
            if (m_NextIndex >= NumMaxKeywords)
            {
                LOG_WARNING("Keyword count exceeds {}; '{}' is ignored!", NumMaxKeywords, ShaderUtils::GetStringFromId(keywordId));
                return false;
            }

            m_KeywordIndexMap[keywordId] = m_NextIndex++;
        }

        return true;
    }

    std::optional<size_t> ShaderKeywordSpace::GetKeywordIndex(const std::string& keyword) const
    {
        return GetKeywordIndex(ShaderUtils::GetIdFromString(keyword));
    }

    std::optional<size_t> ShaderKeywordSpace::GetKeywordIndex(int32 keywordId) const
    {
        if (auto it = m_KeywordIndexMap.find(keywordId); it != m_KeywordIndexMap.end())
        {
            return static_cast<size_t>(it->second);
        }

        return std::nullopt;
    }

    const std::string& ShaderKeywordSpace::GetKeywordString(size_t index) const
    {
        return ShaderUtils::GetStringFromId(GetKeywordId(index));
    }

    int32 ShaderKeywordSpace::GetKeywordId(size_t index) const
    {
        for (const auto& p : m_KeywordIndexMap)
        {
            if (p.second == index)
            {
                return p.first;
            }
        }

        throw std::invalid_argument(StringUtils::Format("Invalid keyword index: {}", index));
    }

    std::vector<std::string> ShaderKeywordSet::GetEnabledKeywordStrings(const ShaderKeywordSpace& space) const
    {
        std::vector<std::string> results{};

        for (size_t i = 0; i < m_Keywords.size(); i++)
        {
            if (m_Keywords[i])
            {
                results.push_back(space.GetKeywordString(i));
            }
        }

        return results;
    }

    std::vector<int32> ShaderKeywordSet::GetEnabledKeywordIds(const ShaderKeywordSpace& space) const
    {
        std::vector<int32> results{};

        for (size_t i = 0; i < m_Keywords.size(); i++)
        {
            if (m_Keywords[i])
            {
                results.push_back(space.GetKeywordId(i));
            }
        }

        return results;
    }

    void ShaderKeywordSet::SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value)
    {
        SetKeyword(space, ShaderUtils::GetIdFromString(keyword), value);
    }

    void ShaderKeywordSet::SetKeyword(const ShaderKeywordSpace& space, int32 keywordId, bool value)
    {
        if (std::optional<size_t> i = space.GetKeywordIndex(keywordId))
        {
            m_Keywords[*i] = value;
        }
    }

    void DynamicShaderKeywordSet::TransformToSpace(const ShaderKeywordSpace& space)
    {
        m_KeywordSet.Clear();

        for (int32 id : m_EnabledKeywordIds)
        {
            m_KeywordSet.EnableKeyword(space, id);
        }
    }

    void DynamicShaderKeywordSet::SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value)
    {
        SetKeyword(space, ShaderUtils::GetIdFromString(keyword), value);
    }

    void DynamicShaderKeywordSet::SetKeyword(const ShaderKeywordSpace& space, int32 keywordId, bool value)
    {
        if (value)
        {
            if (m_EnabledKeywordIds.insert(keywordId).second)
            {
                m_KeywordSet.SetKeyword(space, keywordId, value);
            }
        }
        else
        {
            if (m_EnabledKeywordIds.erase(keywordId) == 1)
            {
                m_KeywordSet.SetKeyword(space, keywordId, value);
            }
        }
    }
}
