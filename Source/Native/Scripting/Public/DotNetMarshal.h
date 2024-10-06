#pragma once

#include "DotNetTypeTraits.h"
#include <DirectXMath.h>
#include <stdint.h>
#include <string>
#include <utility>
#include <stdexcept>

namespace march
{
    template<typename T, typename = void>
    struct cs;

    // blittable 类型
    template<typename T>
    struct cs<T, std::enable_if_t<is_blittable_v<T>>> final
    {
        using managed_type = typename T;

        T data;

        void assign(const T& value) { data = value; }
        void assign(T&& value) { data = std::move(value); }

        constexpr T* operator &() { return &data; }
        constexpr const T* operator &() const { return &data; }
        constexpr operator T () const { return data; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr U operator ->() const { return data; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr U operator +(ptrdiff_t offset) const { return data + offset; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr U operator -(ptrdiff_t offset) const { return data - offset; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr std::remove_pointer_t<U>& operator *() const { return *data; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr std::remove_pointer_t<U>& operator [](ptrdiff_t offset) const { return data[offset]; }

        template<typename U = T, typename = std::enable_if_t<std::is_pointer_v<U>>>
        constexpr bool operator ==(std::nullptr_t) const { return data == nullptr; }
    };

    // enum 类型
    template<typename T>
    struct cs<T, std::enable_if_t<std::is_enum_v<T>>> final
    {
        using managed_type = int32_t;

        managed_type data;

        void assign(const T& value) { data = static_cast<managed_type>(value); }
        void assign(T&& value) { data = static_cast<managed_type>(value); }

        constexpr operator T () const { return static_cast<T>(data); }
    };

    template<>
    struct cs<DirectX::XMFLOAT2> final
    {
        using managed_type = struct
        {
            float x;
            float y;
        };

        managed_type data;

        void assign(const DirectX::XMFLOAT2& value)
        {
            data.x = value.x;
            data.y = value.y;
        }

        void assign(DirectX::XMFLOAT2&& value)
        {
            data.x = value.x;
            data.y = value.y;
        }

        operator DirectX::XMFLOAT2() const
        {
            return { data.x, data.y };
        }
    };

    template<>
    struct cs<DirectX::XMFLOAT3> final
    {
        using managed_type = struct
        {
            float x;
            float y;
            float z;
        };

        managed_type data;

        void assign(const DirectX::XMFLOAT3& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
        }

        void assign(DirectX::XMFLOAT3&& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
        }

        operator DirectX::XMFLOAT3() const
        {
            return { data.x, data.y, data.z };
        }
    };

    template<>
    struct cs<DirectX::XMFLOAT4> final
    {
        using managed_type = struct
        {
            float x;
            float y;
            float z;
            float w;
        };

        managed_type data;

        void assign(const DirectX::XMFLOAT4& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
            data.w = value.w;
        }

        void assign(DirectX::XMFLOAT4&& value)
        {
            data.x = value.x;
            data.y = value.y;
            data.z = value.z;
            data.w = value.w;
        }

        operator DirectX::XMFLOAT4() const
        {
            return { data.x, data.y, data.z, data.w };
        }
    };

    template<>
    struct cs<DirectX::XMFLOAT4X4> final
    {
        using managed_type = struct
        {
            float m[4][4];
        };

        managed_type data;

        void assign(const DirectX::XMFLOAT4X4& value)
        {
            copy_float4x4(data.m, value.m);
        }

        void assign(DirectX::XMFLOAT4X4&& value)
        {
            copy_float4x4(data.m, value.m);
        }

        operator DirectX::XMFLOAT4X4() const
        {
            DirectX::XMFLOAT4X4 result{};
            copy_float4x4(result.m, data.m);
            return result;
        }

        static void copy_float4x4(float dst[4][4], const float src[4][4])
        {
            float* d = reinterpret_cast<float*>(dst);
            const float* s = reinterpret_cast<const float*>(src);
            std::copy_n(s, 16, d);
        }
    };

    template<>
    struct cs<void> final
    {
        using managed_type = void;

        void assign(void) {}
    };

    template<>
    struct cs<std::string> final
    {
        using managed_type = std::string*;

        managed_type data;

        // 只能在当前对象未持有数据时调用
        void assign(const std::string& value)
        {
            data = create_data(value);
        }

        // 只能在当前对象未持有数据时调用
        void assign(std::string&& value)
        {
            data = create_data(std::move(value));
        }

        operator const std::string& () const { return *data; }

        static managed_type create_data()
        {
            return new typename std::remove_pointer_t<managed_type>();
        }

        static managed_type create_data(const std::string& value)
        {
            return new typename std::remove_pointer_t<managed_type>(value);
        }

        static managed_type create_data(std::string&& value)
        {
            return new typename std::remove_pointer_t<managed_type>(value);
        }

        static void destroy(cs value)
        {
            delete value.data;
        }
    };

    template<typename T>
    struct cs<T[]> final
    {
        using managed_type = struct
        {
            int32_t count;
            uint8_t b; // uint8_t b[];
        }*;

        managed_type data;

        int32_t size() const { return static_cast<int32_t>(data->count / sizeof(T)); }
        T* begin() const { return reinterpret_cast<T*>(&data->b); }
        T* end() const { return reinterpret_cast<T*>(&data->b + data->count); }

        void copy_from(const T* p)
        {
            std::copy(p, p + size(), begin());
        }

        // 只能在当前对象未持有数据时调用
        void assign(int32_t length)
        {
            data = create_data(length);
        }

        // 只能在当前对象未持有数据时调用
        void assign(int32_t length, const T* p)
        {
            assign(length);
            copy_from(p);
        }

        T& operator [](int32_t index) const
        {
            if (index < 0 || index >= size())
            {
                throw std::out_of_range("Index out of range");
            }

            return reinterpret_cast<T*>(&data->b)[index];
        }

        static managed_type create_data(int32_t length)
        {
            int32_t byteCount = sizeof(T) * length;
            auto result = static_cast<managed_type>(malloc(sizeof(int32_t) + static_cast<size_t>(byteCount)));

            if (result != nullptr)
            {
                result->count = byteCount;
            }

            return result;
        }

        static void destroy(cs value)
        {
            if (value.data != nullptr)
            {
                free(value.data);
            }
        }
    };

    using cs_void = cs<void>;
    using cs_byte = cs<uint8_t>;
    using cs_sbyte = cs<int8_t>;
    using cs_ushort = cs<uint16_t>;
    using cs_short = cs<int16_t>;
    using cs_uint = cs<uint32_t>;
    using cs_int = cs<int32_t>;
    using cs_ulong = cs<uint64_t>;
    using cs_long = cs<int64_t>;
    using cs_char = cs<wchar_t>;
    using cs_float = cs<float>;
    using cs_double = cs<double>;
    using cs_bool = cs<bool>;
    using cs_string = cs<std::string>;
    using cs_vec2 = cs<DirectX::XMFLOAT2>;
    using cs_vec3 = cs<DirectX::XMFLOAT3>;
    using cs_vec4 = cs<DirectX::XMFLOAT4>;
    using cs_mat4 = cs<DirectX::XMFLOAT4X4>;
    using cs_quat = cs<DirectX::XMFLOAT4>;
    using cs_color = cs<DirectX::XMFLOAT4>;

    template<typename T>
    using cs_array = cs<T[]>;

    using cs_void_t = cs_void::managed_type;
    using cs_byte_t = cs_byte::managed_type;
    using cs_sbyte_t = cs_sbyte::managed_type;
    using cs_ushort_t = cs_ushort::managed_type;
    using cs_short_t = cs_short::managed_type;
    using cs_uint_t = cs_uint::managed_type;
    using cs_int_t = cs_int::managed_type;
    using cs_ulong_t = cs_ulong::managed_type;
    using cs_long_t = cs_long::managed_type;
    using cs_char_t = cs_char::managed_type;
    using cs_float_t = cs_float::managed_type;
    using cs_double_t = cs_double::managed_type;
    using cs_bool_t = cs_bool::managed_type;
    using cs_string_t = cs_string::managed_type;
    using cs_vec2_t = cs_vec2::managed_type;
    using cs_vec3_t = cs_vec3::managed_type;
    using cs_vec4_t = cs_vec4::managed_type;
    using cs_mat4_t = cs_mat4::managed_type;
    using cs_quat_t = cs_quat::managed_type;
    using cs_color_t = cs_color::managed_type;

    template<typename T>
    using cs_array_t = typename cs_array<T>::managed_type;

    static_assert(std::is_pod_v<cs_void>, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_byte> && sizeof(cs_byte) == 1, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_sbyte> && sizeof(cs_sbyte) == 1, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_ushort> && sizeof(cs_ushort) == 2, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_short> && sizeof(cs_short) == 2, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_uint> && sizeof(cs_uint) == 4, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_int> && sizeof(cs_int) == 4, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_ulong> && sizeof(cs_ulong) == 8, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_long> && sizeof(cs_long) == 8, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_char> && sizeof(cs_char) == 2, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_float> && sizeof(cs_float) == 4, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_double> && sizeof(cs_double) == 8, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_bool> && sizeof(cs_bool) == 1, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_string>, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_vec2> && sizeof(cs_vec2) == 8, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_vec3> && sizeof(cs_vec3) == 12, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_vec4> && sizeof(cs_vec4) == 16, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_mat4> && sizeof(cs_mat4) == 64, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_quat> && sizeof(cs_quat) == 16, "Type must be a POD type");
    static_assert(std::is_pod_v<cs_color> && sizeof(cs_color) == 16, "Type must be a POD type");

    template<typename T>
    constexpr bool is_cs_type_v = false;

    template<typename T>
    constexpr bool is_cs_type_v<cs<T>> = true;

    struct cs_convert final
    {
        template<typename T>
        constexpr auto operator <<(T&& value)
        {
            using pure_t = typename std::remove_cv_t<std::remove_reference_t<T>>;

            if constexpr (is_cs_type_v<pure_t>)
            {
                return value.data;
            }
            else
            {
                cs<pure_t> result{};
                result.assign(std::forward<T>(value));
                return result.data;
            }
        }
    };
}
