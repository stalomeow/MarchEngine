#pragma once

#include <type_traits>
#include <stdint.h>
#include <DirectXMath.h>

namespace march
{
    // https://learn.microsoft.com/en-us/dotnet/standard/native-interop/best-practices#common-windows-data-types

    template<typename T>
    constexpr bool is_blittable_v = std::is_pointer_v<T>;

    template<> constexpr bool is_blittable_v<int8_t> = true;
    template<> constexpr bool is_blittable_v<int16_t> = true;
    template<> constexpr bool is_blittable_v<int32_t> = true;
    template<> constexpr bool is_blittable_v<int64_t> = true;
    template<> constexpr bool is_blittable_v<uint8_t> = true;
    template<> constexpr bool is_blittable_v<uint16_t> = true;
    template<> constexpr bool is_blittable_v<uint32_t> = true;
    template<> constexpr bool is_blittable_v<uint64_t> = true;
    template<> constexpr bool is_blittable_v<float> = true;
    template<> constexpr bool is_blittable_v<double> = true;
    template<> constexpr bool is_blittable_v<DirectX::XMFLOAT2> = true;
    template<> constexpr bool is_blittable_v<DirectX::XMFLOAT3> = true;
    template<> constexpr bool is_blittable_v<DirectX::XMFLOAT4> = true;
    template<> constexpr bool is_blittable_v<DirectX::XMFLOAT4X4> = true;

    // https://learn.microsoft.com/en-us/cpp/cpp/fundamental-types-cpp?view=msvc-170#sizes-of-built-in-types
    template<> constexpr bool is_blittable_v<bool> = std::bool_constant<sizeof(bool) == 1>::value;
    template<> constexpr bool is_blittable_v<wchar_t> = std::bool_constant<sizeof(wchar_t) == 2>::value;

    template<typename T>
    struct is_blittable : std::bool_constant<is_blittable_v<T>> {};
}
