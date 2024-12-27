#include "pch.h"
#include "Engine/Graphics/Shader.h"
#include "Engine/Debug.h"

namespace march
{
    ShaderKeywordSet::ShaderKeywordSet() : m_Keywords{}
    {
    }

    size_t ShaderKeywordSet::GetEnabledKeywordCount() const
    {
        return m_Keywords.count();
    }

    size_t ShaderKeywordSet::GetMatchingKeywordCount(const ShaderKeywordSet& other) const
    {
        return (m_Keywords & other.m_Keywords).count();
    }

    std::vector<std::string> ShaderKeywordSet::GetEnabledKeywords(const ShaderKeywordSpace& space) const
    {
        std::vector<std::string> results{};

        for (size_t i = 0; i < m_Keywords.size(); i++)
        {
            if (m_Keywords[i])
            {
                results.push_back(space.GetKeywordName(static_cast<int8_t>(i)));
            }
        }

        return results;
    }

    void ShaderKeywordSet::SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value)
    {
        if (int8_t i = space.GetKeywordIndex(keyword); i >= 0)
        {
            m_Keywords[static_cast<size_t>(i)] = value;
        }
    }

    void ShaderKeywordSet::EnableKeyword(const ShaderKeywordSpace& space, const std::string& keyword)
    {
        SetKeyword(space, keyword, true);
    }

    void ShaderKeywordSet::DisableKeyword(const ShaderKeywordSpace& space, const std::string& keyword)
    {
        SetKeyword(space, keyword, false);
    }

    void ShaderKeywordSet::Clear()
    {
        m_Keywords.reset();
    }

    ShaderKeywordSpace::ShaderKeywordSpace() : m_KeywordIndexMap{}, m_NextIndex(0)
    {
    }

    size_t ShaderKeywordSpace::GetKeywordCount() const
    {
        return m_KeywordIndexMap.size();
    }

    int8_t ShaderKeywordSpace::GetKeywordIndex(const std::string& keyword) const
    {
        if (auto it = m_KeywordIndexMap.find(keyword); it != m_KeywordIndexMap.end())
        {
            return static_cast<int8_t>(it->second);
        }

        return -1;
    }

    const std::string& ShaderKeywordSpace::GetKeywordName(int8_t index) const
    {
        for (const auto& [name, i] : m_KeywordIndexMap)
        {
            if (i == index)
            {
                return name;
            }
        }

        throw std::runtime_error("Invalid shader keyword index");
    }

    ShaderKeywordSpace::AddKeywordResult ShaderKeywordSpace::AddKeyword(const std::string& keyword)
    {
        if (m_KeywordIndexMap.count(keyword) > 0)
        {
            return AddKeywordResult::AlreadyExists;
        }

        if (m_NextIndex >= 128)
        {
            LOG_WARNING("Keyword count exceeds 128; '{}' is ignored!", keyword);
            return AddKeywordResult::OutOfSpace;
        }

        m_KeywordIndexMap[keyword] = m_NextIndex++;
        return AddKeywordResult::Success;
    }

    void ShaderKeywordSpace::Clear()
    {
        m_KeywordIndexMap.clear();
        m_NextIndex = 0;
    }
}
