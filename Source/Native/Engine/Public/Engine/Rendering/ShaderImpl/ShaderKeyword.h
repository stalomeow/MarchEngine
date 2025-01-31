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

        size_t GetNumKeywords() const { return m_KeywordIndexMap.size(); }
    };

    class ShaderKeywordSet
    {
        friend ::std::hash<ShaderKeywordSet>;

        const ShaderKeywordSpace* m_Space = nullptr;
        std::bitset<ShaderKeywordSpace::NumMaxKeywords> m_Keywords{};

    public:
        std::vector<std::string> GetEnabledKeywordStringsInSpace() const;
        std::vector<int32> GetEnabledKeywordIdsInSpace() const;

        void SetKeyword(const std::string& keyword, bool value);
        void SetKeyword(int32 keywordId, bool value);

        void EnableKeyword(const std::string& keyword) { SetKeyword(keyword, true); }

        void EnableKeyword(int32 keywordId) { SetKeyword(keywordId, true); }

        void DisableKeyword(const std::string& keyword) { SetKeyword(keyword, false); }

        void DisableKeyword(int32 keywordId) { SetKeyword(keywordId, false); }

        const ShaderKeywordSpace* GetSpace() const { return m_Space; }

        void Reset(const ShaderKeywordSpace* space)
        {
            m_Space = space;
            m_Keywords.reset();
        }

        size_t ShaderKeywordSet::GetNumEnabledKeywords() const
        {
            return m_Keywords.count();
        }

        size_t ShaderKeywordSet::GetNumMatchingKeywords(const ShaderKeywordSet& other) const
        {
            if (m_Space != other.m_Space)
            {
                return 0;
            }

            return (m_Keywords & other.m_Keywords).count();
        }

        bool operator==(const ShaderKeywordSet& other) const
        {
            return m_Space == other.m_Space && m_Keywords == other.m_Keywords;
        }

        bool operator!=(const ShaderKeywordSet& other) const
        {
            return !(*this == other);
        }
    };

    class DynamicShaderKeywordSet
    {
        std::unordered_set<int32> m_EnabledKeywordIds{};
        ShaderKeywordSet m_KeywordSet{};

    public:
        void TransformToSpace(const ShaderKeywordSpace* space);

        std::vector<std::string> GetEnabledKeywordStrings() const;
        std::vector<int32> GetEnabledKeywordIds() const;

        void SetKeyword(const std::string& keyword, bool value);
        void SetKeyword(int32 keywordId, bool value);

        void EnableKeyword(const std::string& keyword) { SetKeyword(keyword, true); }

        void EnableKeyword(int32 keywordId) { SetKeyword(keywordId, true); }

        void DisableKeyword(const std::string& keyword) { SetKeyword(keyword, false); }

        void DisableKeyword(int32 keywordId) { SetKeyword(keywordId, false); }

        const ShaderKeywordSet& GetKeywords() const { return m_KeywordSet; }

        void Clear()
        {
            m_EnabledKeywordIds.clear();
            m_KeywordSet.Reset(m_KeywordSet.GetSpace());
        }
    };
}

template <>
struct std::hash<march::ShaderKeywordSet>
{
    size_t operator()(const march::ShaderKeywordSet& keywords) const
    {
        size_t hash1 = std::hash<decltype(keywords.m_Space)>{}(keywords.m_Space);
        size_t hash2 = std::hash<decltype(keywords.m_Keywords)>{}(keywords.m_Keywords);
        return hash1 ^ hash2;
    }
};
