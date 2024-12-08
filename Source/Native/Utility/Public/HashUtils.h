#pragma once

#include <stdint.h>
#include <assert.h>

namespace march
{
    namespace HashUtils
    {
        // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Hash.h

        inline size_t FNV1Range(const uint32_t* const Begin, const uint32_t* const End, size_t Hash)
        {
            for (const uint32_t* Iter = Begin; Iter < End; ++Iter)
            {
                Hash = 16777619U * Hash ^ *Iter;
            }

            return Hash;
        }

        constexpr size_t DefaultHash = 2166136261U;

        template <typename T>
        inline size_t FNV1(const T* Object, size_t Count = 1, size_t Hash = DefaultHash)
        {
            static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "Object is not word-aligned");
            return FNV1Range(reinterpret_cast<const uint32_t*>(Object), reinterpret_cast<const uint32_t*>(Object + Count), Hash);
        }

        template <>
        inline size_t FNV1<uint8_t>(const uint8_t* Object, size_t Count, size_t Hash)
        {
            assert((Count & 3) == 0); // Count must be a multiple of 4
            return FNV1Range(reinterpret_cast<const uint32_t*>(Object), reinterpret_cast<const uint32_t*>(Object + Count), Hash);
        }
    }

    class FNV1Hash
    {
    public:
        template <typename T>
        void Append(const T* Object, size_t Count = 1)
        {
            HashUtils::FNV1(Object, Count, m_Value);
        }

        size_t GetValue() const { return m_Value; }

    private:
        size_t m_Value = HashUtils::DefaultHash;
    };
}
