#pragma once

#include "Engine/Ints.h"
#include <atomic>

namespace march
{
    class MarchObject
    {
    public:
        virtual ~MarchObject() = default;
    };
}

// https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library?view=msvc-170
#ifdef _DEBUG
    #include <crtdbg.h>
    #define MARCH_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
    // Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the allocations to be of _CLIENT_BLOCK type
#else
    #define MARCH_NEW new
#endif

#define MARCH_MAKE_REF(T, ...) ::march::RefCountPtr<T>().Attach(MARCH_NEW T(__VA_ARGS__))

template <typename T>
struct std::hash<march::RefCountPtr<T>>
{
    auto operator()(const march::RefCountPtr<T>& ptr) const noexcept
    {
        return std::hash<T*>{}(ptr.Get());
    }
};
