#pragma once

#include <stdint.h>
#include <assert.h>

namespace march
{
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Hash.h

    class FNV1Hash
    {
        size_t m_Value = 2166136261U;

        void AppendRange(const uint32_t* const begin, const uint32_t* const end)
        {
            for (const uint32_t* iter = begin; iter < end; ++iter)
            {
                m_Value = 16777619U * m_Value ^ *iter;
            }
        }

    public:
        size_t GetValue() const { return m_Value; }

        template <typename T>
        void Append(const T& obj)
        {
            static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "Object is not word-aligned");

            const T* ptr = &obj;
            auto begin = reinterpret_cast<const uint32_t*>(ptr);
            auto end = reinterpret_cast<const uint32_t*>(ptr + 1);
            return AppendRange(begin, end);
        }

        void Append(const void* data, size_t sizeInBytes)
        {
            assert((sizeInBytes & 3) == 0 && "sizeInBytes must be a multiple of 4");

            auto begin = reinterpret_cast<const uint32_t*>(data);
            auto end = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(data) + sizeInBytes);
            return AppendRange(begin, end);
        }
    };

    using DefaultHash = typename FNV1Hash;
}
