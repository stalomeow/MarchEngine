#pragma once

#include "Engine/Ints.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <optional>
#include <bitset>
#include <vector>

namespace march
{
    class ShaderKeywordSpace
    {
        std::unordered_map<int32, uint8> m_KeywordIndexMap{};
        uint8 m_NextIndex = 0;

    public:
        static constexpr size_t NumMaxKeywords = 128; // 目前最多支持 128 个 Keyword

        bool RegisterKeyword(const std::string& keyword);
        bool RegisterKeyword(int32 keywordId);

        std::optional<size_t> GetKeywordIndex(const std::string& keyword) const;
        std::optional<size_t> GetKeywordIndex(int32 keywordId) const;

        const std::string& GetKeywordString(size_t index) const;
        int32 GetKeywordId(size_t index) const;

        size_t GetNumKeywords() const
        {
            return m_KeywordIndexMap.size();
        }

        void Reset()
        {
            m_KeywordIndexMap.clear();
            m_NextIndex = 0;
        }
    };

    class ShaderKeywordSet
    {
        friend ::std::hash<ShaderKeywordSet>;

        std::bitset<ShaderKeywordSpace::NumMaxKeywords> m_Keywords{};

    public:
        std::vector<std::string> GetEnabledKeywordStrings(const ShaderKeywordSpace& space) const;
        std::vector<int32> GetEnabledKeywordIds(const ShaderKeywordSpace& space) const;

        void SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value);
        void SetKeyword(const ShaderKeywordSpace& space, int32 keywordId, bool value);

        void EnableKeyword(const ShaderKeywordSpace& space, const std::string& keyword)
        {
            SetKeyword(space, keyword, true);
        }

        void EnableKeyword(const ShaderKeywordSpace& space, int32 keywordId)
        {
            SetKeyword(space, keywordId, true);
        }

        void DisableKeyword(const ShaderKeywordSpace& space, const std::string& keyword)
        {
            SetKeyword(space, keyword, false);
        }

        void DisableKeyword(const ShaderKeywordSpace& space, int32 keywordId)
        {
            SetKeyword(space, keywordId, false);
        }

        void Clear()
        {
            m_Keywords.reset();
        }

        size_t ShaderKeywordSet::GetNumEnabledKeywords() const
        {
            return m_Keywords.count();
        }

        size_t ShaderKeywordSet::GetNumMatchingKeywords(const ShaderKeywordSet& other) const
        {
            return (m_Keywords & other.m_Keywords).count();
        }

        bool operator==(const ShaderKeywordSet& other) const
        {
            return m_Keywords == other.m_Keywords;
        }

        bool operator!=(const ShaderKeywordSet& other) const
        {
            return m_Keywords != other.m_Keywords;
        }
    };

    class DynamicShaderKeywordSet
    {
        std::unordered_set<int32> m_EnabledKeywordIds{};
        ShaderKeywordSet m_KeywordSet{};

    public:
        void TransformToSpace(const ShaderKeywordSpace& space);

        void SetKeyword(const ShaderKeywordSpace& space, const std::string& keyword, bool value);
        void SetKeyword(const ShaderKeywordSpace& space, int32 keywordId, bool value);

        void EnableKeyword(const ShaderKeywordSpace& space, const std::string& keyword)
        {
            SetKeyword(space, keyword, true);
        }

        void EnableKeyword(const ShaderKeywordSpace& space, int32 keywordId)
        {
            SetKeyword(space, keywordId, true);
        }

        void DisableKeyword(const ShaderKeywordSpace& space, const std::string& keyword)
        {
            SetKeyword(space, keyword, false);
        }

        void DisableKeyword(const ShaderKeywordSpace& space, int32 keywordId)
        {
            SetKeyword(space, keywordId, false);
        }

        void Clear()
        {
            m_EnabledKeywordIds.clear();
            m_KeywordSet.Clear();
        }

        const ShaderKeywordSet& GetKeywordSet() const
        {
            return m_KeywordSet;
        }
    };
}

template <>
struct std::hash<march::ShaderKeywordSet>
{
    size_t operator()(const march::ShaderKeywordSet& keywords) const
    {
        return std::hash<decltype(keywords.m_Keywords)>{}(keywords.m_Keywords);
    }
};
