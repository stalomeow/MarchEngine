#pragma once

#include <type_traits>
#include <stdint.h>
#include <DirectXMath.h>

namespace march
{
    // https://learn.microsoft.com/en-us/dotnet/standard/native-interop/best-practices#common-windows-data-types

    template<typename T, typename = void>
    inline constexpr bool is_blittable_v = false;

    template<> inline constexpr bool is_blittable_v<int8_t> = true;
    template<> inline constexpr bool is_blittable_v<int16_t> = true;
    template<> inline constexpr bool is_blittable_v<int32_t> = true;
    template<> inline constexpr bool is_blittable_v<int64_t> = true;
    template<> inline constexpr bool is_blittable_v<uint8_t> = true;
    template<> inline constexpr bool is_blittable_v<uint16_t> = true;
    template<> inline constexpr bool is_blittable_v<uint32_t> = true;
    template<> inline constexpr bool is_blittable_v<uint64_t> = true;
    template<> inline constexpr bool is_blittable_v<float> = true;
    template<> inline constexpr bool is_blittable_v<double> = true;

    // https://learn.microsoft.com/en-us/cpp/cpp/fundamental-types-cpp?view=msvc-170#sizes-of-built-in-types
    template<> inline constexpr bool is_blittable_v<bool> = false; // 开 Optimization 后，C++ 和 C# 的 bool 返回值行为不一致
    template<> inline constexpr bool is_blittable_v<wchar_t> = std::bool_constant<sizeof(wchar_t) == 2>::value;

    // 指针
    template<typename T>
    inline constexpr bool is_blittable_v<T, std::enable_if_t<std::is_pointer_v<T>>> = true;

    // 普通 struct
    template<typename T>
    inline constexpr bool is_blittable_v<T, std::enable_if_t<std::is_class_v<T>
        && std::is_pod_v<T>
        && std::is_standard_layout_v<T>
        && std::is_trivially_copyable_v<T>>> = true;

    // 检查 NativeType 是否 blittable
    template<typename T>
    struct is_blittable : std::bool_constant<is_blittable_v<T>> {};
}
