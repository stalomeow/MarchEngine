#pragma once

#include <stdint.h>

namespace march::HashUtils
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

    template <typename T>
    inline size_t FNV1(const T* Object, size_t Count = 1, size_t Hash = 2166136261U)
    {
        static_assert((sizeof(T) & 3) == 0 && alignof(T) >= 4, "Object is not word-aligned");
        return FNV1Range(reinterpret_cast<const uint32_t*>(Object), reinterpret_cast<const uint32_t*>(Object + Count), Hash);
    }
}
